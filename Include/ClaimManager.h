#pragma once
// ClaimManager = everything that is still "pending" from the Phase-II report:
//   private detail + follow-up question, claim submission and answer check (Levenshtein),
//   trie word search, category/location hash index, admin login,
//   claim priority queue (ID cards & keys first), approve / reject,
//   stack log of admin decisions (+ undo), sorting for admin views,
//   statistics dashboard, and save / reload of claims and decisions.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <queue>
#include <string>
#include <vector>

#include "Claim.h"
#include "ItemIndex.h"
#include "Levenshtein.h"
#include "LostFoundSystem.h"
#include "MatchingEngine.h"
#include "Sorting.h"
#include "Trie.h"
#include "stack.h"

class ClaimManager {
    // demo admin account (a real system would not hard-code this)
    static constexpr const char* ADMIN_USER = "admin";
    static constexpr const char* ADMIN_PASS = "geu@2026";

    LostFoundSystem& sys_;
    ItemIndex index_;   // hash table (C): category / location -> item IDs
    Trie trie_;         // prefix search over words in the reports
    std::vector<Claim> claims_;
    Stack* log_;        // stack (C): admin decisions, newest on top
    std::string claimsFile_, decisionsFile_;
    int nextSeq_ = 1;

    // ---- claim priority queue: ID cards/keys first, then oldest claim first ----
    struct QEntry { int prio; int seq; std::string claimId; };
    struct QCmp {
        bool operator()(const QEntry& a, const QEntry& b) const {
            if (a.prio != b.prio) return a.prio > b.prio;  // smaller prio value = higher priority
            return a.seq > b.seq;
        }
    };
    std::priority_queue<QEntry, std::vector<QEntry>, QCmp> queue_;

    // ================= small helpers =================
    Claim* findClaim(const std::string& id) {
        for (auto& c : claims_) if (c.id == id) return &c;
        return nullptr;
    }

    void pushQueue(const Claim& c) {
        Item* it = sys_.findById(c.itemId);
        int prio = it ? static_cast<int>(it->priority()) : static_cast<int>(Priority::LOW);
        queue_.push({prio, c.seq, c.id});
    }

    void rebuildQueue() {
        queue_ = decltype(queue_)();
        for (const auto& c : claims_) if (c.isOpen()) pushQueue(c);
    }

    void dropStale() {  // remove queue entries whose claim is no longer open
        while (!queue_.empty()) {
            Claim* c = findClaim(queue_.top().claimId);
            if (c && c->isOpen()) break;
            queue_.pop();
        }
    }

    static std::string timeStr(long t) {
        std::time_t tt = static_cast<std::time_t>(t);
        char buf[32];
        std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", std::localtime(&tt));
        return buf;
    }

    static void ensureDir(const std::string& file) {
        std::filesystem::path p(file);
        if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    }

    void indexItem(const Item* it) {
        index_.add(*it);
        trie_.insertText(it->name() + " " + it->category() + " " + it->color() + " " +
                             it->location() + " " + it->description(),
                         it->id());
        // note: the PRIVATE detail is never indexed or searchable
    }

    void printClaimLine(int rank, const Claim& c) {
        Item* it = sys_.findById(c.itemId);
        std::cout << " #" << rank << "  " << c.id << " | "
                  << (it ? priorityToString(it->priority()) : "?") << " | item " << c.itemId
                  << (it ? " (" + it->category() + ")" : "") << " | claimant: " << c.claimant
                  << " | " << claimStatusToString(c.status) << " | answer match " << c.similarityPct << "%\n";
    }

    // ================= persistence =================
    void saveClaims() const {
        ensureDir(claimsFile_);
        std::ofstream out(claimsFile_, std::ios::trunc);
        for (const auto& c : claims_) out << c.serialize() << "\n";
    }

    static void collectDecisionCb(const Decision* d, void* ctx) {
        static_cast<std::vector<Decision>*>(ctx)->push_back(*d);
    }

    void saveDecisions() const {
        ensureDir(decisionsFile_);
        std::vector<Decision> v;
        stack_foreach(log_, collectDecisionCb, &v);  // newest -> oldest
        std::reverse(v.begin(), v.end());            // write oldest first
        std::ofstream out(decisionsFile_, std::ios::trunc);
        for (const auto& d : v)
            out << d.claimId << "\t" << d.itemId << "\t" << d.claimant << "\t" << d.admin << "\t"
                << d.approved << "\t" << d.when << "\t" << d.note << "\n";
    }

    void saveAll() const {
        sys_.save();
        saveClaims();
        saveDecisions();
    }

    void loadClaims() {
        std::ifstream in(claimsFile_);
        std::string line;
        while (in && std::getline(in, line)) {
            Claim c;
            if (line.empty() || !Claim::parse(line, c)) continue;
            nextSeq_ = std::max(nextSeq_, c.seq + 1);
            claims_.push_back(c);
        }
    }

    void loadDecisions() {
        std::ifstream in(decisionsFile_);
        std::string line;
        while (in && std::getline(in, line)) {
            auto f = util::split(line, '\t');
            if (f.size() < 6) continue;
            Decision d;
            std::memset(&d, 0, sizeof d);
            std::snprintf(d.claimId, sizeof d.claimId, "%s", f[0].c_str());
            std::snprintf(d.itemId, sizeof d.itemId, "%s", f[1].c_str());
            std::snprintf(d.claimant, sizeof d.claimant, "%s", f[2].c_str());
            std::snprintf(d.admin, sizeof d.admin, "%s", f[3].c_str());
            d.approved = std::atoi(f[4].c_str());
            d.when = std::atol(f[5].c_str());
            if (f.size() > 6) std::snprintf(d.note, sizeof d.note, "%s", f[6].c_str());
            stack_push(log_, &d);  // oldest first, so the newest ends on top
        }
    }

    void logDecision(const Claim& c, bool approved) {
        Decision d;
        std::memset(&d, 0, sizeof d);
        std::snprintf(d.claimId, sizeof d.claimId, "%s", c.id.c_str());
        std::snprintf(d.itemId, sizeof d.itemId, "%s", c.itemId.c_str());
        std::snprintf(d.claimant, sizeof d.claimant, "%s", c.claimant.c_str());
        std::snprintf(d.admin, sizeof d.admin, "%s", ADMIN_USER);
        std::snprintf(d.note, sizeof d.note, "%s", c.adminNote.c_str());
        d.approved = approved ? 1 : 0;
        d.when = static_cast<long>(std::time(nullptr));
        stack_push(log_, &d);
    }

public:
    explicit ClaimManager(LostFoundSystem& sys, std::string claimsFile = "data/claims.txt",
                          std::string decisionsFile = "data/decisions.txt")
        : sys_(sys), log_(stack_create()), claimsFile_(std::move(claimsFile)),
          decisionsFile_(std::move(decisionsFile)) {}
    ClaimManager(const ClaimManager&) = delete;
    ClaimManager& operator=(const ClaimManager&) = delete;
    ~ClaimManager() { stack_destroy(log_); }

    // Call AFTER sys.load(): indexes the loaded items, reloads claims + decisions.
    void load() {
        for (const ListNode* p = sys_.items().head(); p; p = p->next) indexItem(p->item);
        loadClaims();
        loadDecisions();
        rebuildQueue();
        std::cout << "Loaded " << claims_.size() << " claim(s), " << stack_size(log_)
                  << " logged decision(s).\n";
    }

    void registerMenus() {
        sys_.setOnItemAdded([this](Item* it) { onItemAdded(it); });
        sys_.addMenuOption("Claim a found item (answer the follow-up question)", [this] { submitClaim(); });
        sys_.addMenuOption("Search items by word / prefix (trie)", [this] { searchPrefix(); });
        sys_.addMenuOption("Browse by category or location (hash index)", [this] { browse(); });
        sys_.addMenuOption("Admin panel (login required)", [this] { adminPanel(); });
    }

    // ================= private detail (called while a report is being added) =================
    void onItemAdded(Item* it) {
        std::cout << "\n--- Private detail (kept hidden, never shown to other students) ---\n";
        std::string detail = util::readNonEmpty("Private detail (e.g. scratch on back cover, red rubber band): ");
        std::string question;
        if (it->type() == "FOUND")
            question = util::readNonEmpty("Follow-up question for the claimant (e.g. What is tied around the charger?): ");
        it->setPrivate(detail, question);
        indexItem(it);
    }

    // ================= student: claim =================
    void submitClaim() {
        std::cout << "\n--- Claim a found item ---\n";
        std::string claimant = util::readNonEmpty("Your name: ");
        std::string itemId = util::readNonEmpty("ID of the FOUND item you want to claim: ");
        Item* it = sys_.findById(itemId);
        if (!it || it->type() != "FOUND") { std::cout << "No FOUND item with ID '" << itemId << "'.\n"; return; }
        if (it->status() != "OPEN") { std::cout << "That item has already been returned to its owner.\n"; return; }
        for (const auto& c : claims_)
            if (c.isOpen() && c.itemId == itemId && c.claimant == claimant) {
                std::cout << "You already have an open claim (" << c.id << ") for this item.\n";
                return;
            }

        Claim c;
        c.seq = nextSeq_++;
        char idbuf[16];
        std::snprintf(idbuf, sizeof idbuf, "C%03d", c.seq);
        c.id = idbuf;
        c.itemId = itemId;
        c.claimant = claimant;

        if (it->question().empty() || it->privateDetail().empty()) {
            std::cout << "No follow-up question on file for this item.\n";
            c.answer = "(none)";
            c.status = ClaimStatus::FLAGGED;
            c.similarityPct = 0;
        } else {
            std::cout << "Follow-up question: " << it->question() << "\n";
            c.answer = util::readNonEmpty("Your answer: ");
            double sim = lev::fieldSimilarity(c.answer, it->privateDetail());
            c.similarityPct = static_cast<int>(sim * 100.0 + 0.5);
            c.status = sim >= MatchingEngine::FIELD_THRESHOLD ? ClaimStatus::PENDING : ClaimStatus::FLAGGED;
        }

        claims_.push_back(c);
        pushQueue(c);
        saveClaims();
        if (c.status == ClaimStatus::PENDING)
            std::cout << "Claim " << c.id << ": your answer lines up with the original report.\n"
                      << "It has been sent to the admin for approval.\n";
        else
            std::cout << "Claim " << c.id << ": your answer did not match closely.\n"
                      << "It is NOT rejected - it is flagged for manual review by the admin.\n";
    }

    // ================= search / browse =================
    void searchPrefix() const {
        std::string word = util::readNonEmpty("Type the start of a word (e.g. bac): ");
        auto ids = trie_.searchPrefix(word);
        if (ids.empty()) { std::cout << "No items contain a word starting with '" << word << "'.\n"; return; }
        std::cout << ids.size() << " item(s) found:\n";
        for (const auto& id : ids) if (Item* it = sys_.findById(id)) it->display();
    }

    void browse() const {
        int mode = util::readInt("Browse by 1) category  2) location : ", 1, 2);
        std::string key = util::readNonEmpty(mode == 1 ? "Category: " : "Location: ");
        auto ids = mode == 1 ? index_.byCategory(key) : index_.byLocation(key);
        if (ids.empty()) { std::cout << "Nothing found for '" << key << "'.\n"; return; }
        for (const auto& id : ids) if (Item* it = sys_.findById(id)) it->display();
    }

    // ================= admin =================
    bool login() const {
        for (int attempt = 1; attempt <= 3; ++attempt) {
            std::string u = util::readLine("Admin username: ");
            std::string p = util::readLine("Password: ");
            if (u == ADMIN_USER && p == ADMIN_PASS) { std::cout << "Welcome, admin.\n"; return true; }
            std::cout << "Wrong username or password (" << attempt << "/3).\n";
            if (!std::cin) break;
        }
        return false;
    }

    void adminPanel() {
        if (!login()) { std::cout << "Access denied.\n"; return; }
        while (std::cin) {
            std::cout << "\n===== ADMIN PANEL =====\n"
                      << " 1. View claim queue (priority order)\n"
                      << " 2. Process next claim (approve / reject)\n"
                      << " 3. View decision log (newest first)\n"
                      << " 4. Undo last decision\n"
                      << " 5. Sorted views\n"
                      << " 6. Statistics dashboard\n"
                      << " 0. Logout\n";
            int c = util::readInt("Choice: ", 0, 6);
            if (c == 0) break;
            switch (c) {
                case 1: viewQueue(); break;
                case 2: processNext(); break;
                case 3: viewLog(); break;
                case 4: undoLast(); break;
                case 5: sortedViews(); break;
                case 6: statistics(); break;
            }
        }
    }

    void viewQueue() {
        dropStale();
        if (queue_.empty()) { std::cout << "No claims waiting.\n"; return; }
        std::cout << "\n--- Claims waiting (ID cards and keys first) ---\n";
        auto copy = queue_;  // pop a COPY so the real queue stays untouched
        int rank = 1;
        while (!copy.empty()) {
            if (Claim* c = findClaim(copy.top().claimId)) if (c->isOpen()) printClaimLine(rank++, *c);
            copy.pop();
        }
    }

    void processNext() {
        dropStale();
        if (queue_.empty()) { std::cout << "No claims waiting.\n"; return; }
        Claim* c = findClaim(queue_.top().claimId);
        Item* item = sys_.findById(c->itemId);

        if (!item || item->status() != "OPEN") {  // someone else already got it
            c->status = ClaimStatus::REJECTED;
            c->adminNote = "Auto-rejected: item already returned";
            queue_.pop();
            logDecision(*c, false);
            saveAll();
            std::cout << "Claim " << c->id << " auto-rejected: the item was already returned.\n";
            return;
        }

        std::cout << "\n--- Next claim: " << c->id << " (" << claimStatusToString(c->status) << ") ---\n";
        item->display();
        std::cout << "Claimant      : " << c->claimant << "\n"
                  << "Question asked: " << item->question() << "\n"
                  << "Their answer  : " << c->answer << "\n"
                  << "Private detail: " << item->privateDetail() << "   (match " << c->similarityPct << "%)\n";
        if (c->status == ClaimStatus::FLAGGED) std::cout << ">> Answer did not match - decide manually.\n";

        std::string a = util::lower(util::readLine("[a]pprove / [r]eject / [s]kip: "));
        if (a != "a" && a != "r") { std::cout << "Left in the queue.\n"; return; }
        bool approve = (a == "a");
        c->adminNote = util::readLine("Note (optional): ");
        c->status = approve ? ClaimStatus::APPROVED : ClaimStatus::REJECTED;
        if (approve) item->setStatus("CLAIMED");
        queue_.pop();
        logDecision(*c, approve);
        saveAll();
        std::cout << "Claim " << c->id << (approve ? " APPROVED - item returned to " + c->claimant : " REJECTED") << ".\n";
    }

    static void printDecisionCb(const Decision* d, void* ctx) {
        int* n = static_cast<int*>(ctx);
        std::cout << " " << ++*n << ". " << timeStr(d->when) << " | " << d->claimId << " | item " << d->itemId
                  << " | " << d->claimant << " | " << (d->approved ? "APPROVED" : "REJECTED");
        if (d->note[0]) std::cout << " | note: " << d->note;
        std::cout << "\n";
    }

    void viewLog() const {
        if (stack_size(log_) == 0) { std::cout << "No decisions logged yet.\n"; return; }
        std::cout << "\n--- Decision log (newest first) ---\n";
        int n = 0;
        stack_foreach(log_, printDecisionCb, &n);
    }

    void undoLast() {
        Decision d;
        if (!stack_pop(log_, &d)) { std::cout << "Nothing to undo.\n"; return; }
        Claim* c = findClaim(d.claimId);
        if (!c) return;
        if (d.approved) if (Item* it = sys_.findById(d.itemId)) it->setStatus("OPEN");
        c->status = c->similarityPct >= static_cast<int>(MatchingEngine::FIELD_THRESHOLD * 100)
                        ? ClaimStatus::PENDING : ClaimStatus::FLAGGED;
        c->adminNote.clear();
        pushQueue(*c);
        saveAll();
        std::cout << "Undid the decision on " << c->id << "; the claim is back in the queue.\n";
    }

    // ================= sorting for admin views =================
    void sortedViews() {
        std::cout << "\n 1. Items by priority\n 2. Items by name (A-Z)\n 3. Items by ID\n"
                  << " 4. Claims by answer match (best first)\n"
                  << " 5. Binary search an item by ID (on the ID-sorted list)\n";
        int k = util::readInt("Choice: ", 1, 5);
        if (k == 4) {
            std::vector<Claim> v = claims_;
            if (v.empty()) { std::cout << "No claims yet.\n"; return; }
            sorting::mergeSort(v, [](const Claim& a, const Claim& b) { return a.similarityPct > b.similarityPct; });
            int r = 1;
            for (const auto& c : v) printClaimLine(r++, c);
            return;
        }
        std::vector<Item*> v;
        for (const ListNode* p = sys_.items().head(); p; p = p->next) v.push_back(p->item);
        if (v.empty()) { std::cout << "No items yet.\n"; return; }
        // merge sort for priority (it is STABLE: same-priority items keep their order);
        // quick sort for name / ID (keys are different, so stability does not matter)
        if (k == 1)
            sorting::mergeSort(v, [](const Item* a, const Item* b) { return a->priority() < b->priority(); });
        else if (k == 2)
            sorting::quickSort(v, [](const Item* a, const Item* b) { return util::lower(a->name()) < util::lower(b->name()); });
        else
            sorting::quickSort(v, [](const Item* a, const Item* b) { return a->id() < b->id(); });

        if (k == 5) {  // binary search needs the list sorted by ID (done just above)
            std::string id = util::readNonEmpty("Item ID to find: ");
            int steps = 0;
            int pos = sorting::binarySearch(v, id, [](const Item* a) { return a->id(); }, &steps);
            if (pos < 0) std::cout << "Not found (" << steps << " comparisons for " << v.size() << " items).\n";
            else {
                std::cout << "Found at position " << pos + 1 << " of " << v.size() << " after " << steps << " comparison(s).\n";
                v[pos]->display();
            }
            return;
        }
        for (const Item* it : v)
            std::cout << " " << it->id() << " | " << it->type() << " | " << priorityToString(it->priority())
                      << " | " << it->name() << " (" << it->category() << ") | " << it->status() << "\n";
    }

    // ================= statistics dashboard =================
    static void printTop(const char* title, std::vector<std::pair<std::string, int>> v) {
        sorting::mergeSort(v, [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
        std::cout << title << "\n";
        if (v.empty()) std::cout << "   (none)\n";
        for (size_t i = 0; i < v.size() && i < 5; ++i)
            std::cout << "   " << v[i].first << ": " << std::string(static_cast<size_t>(v[i].second), '#')
                      << " " << v[i].second << "\n";
    }

    void statistics() const {
        int lost = 0, found = 0, open = 0, claimed = 0, highOpen = 0;
        for (const ListNode* p = sys_.items().head(); p; p = p->next) {
            const Item* it = p->item;
            (it->type() == "LOST" ? lost : found)++;
            if (it->status() == "OPEN") { open++; if (it->priority() == Priority::HIGH) highOpen++; }
            else claimed++;
        }
        int pending = 0, flagged = 0, approved = 0, rejected = 0;
        for (const auto& c : claims_) {
            switch (c.status) {
                case ClaimStatus::PENDING: pending++; break;
                case ClaimStatus::FLAGGED: flagged++; break;
                case ClaimStatus::APPROVED: approved++; break;
                default: rejected++; break;
            }
        }
        std::cout << "\n===== STATISTICS =====\n"
                  << "Items  : " << (lost + found) << " total (" << lost << " lost, " << found << " found)\n"
                  << "         " << open << " open, " << claimed << " returned, " << highOpen << " high-priority open\n"
                  << "Claims : " << claims_.size() << " total | waiting: " << pending << " verified + "
                  << flagged << " flagged | approved: " << approved << " | rejected: " << rejected << "\n"
                  << "Log    : " << stack_size(log_) << " decision(s)\n";
        printTop("Top categories:", index_.categoryCounts());
        printTop("Top locations:", index_.locationCounts());
    }
};

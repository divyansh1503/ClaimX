#pragma once
// LostFoundSystem: owns the data, runs the menu and connects the other classes.
// Extra modules (claims, admin ...) plug in with addMenuOption() / setOnItemAdded().
#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "FileManager.h"
#include "ItemLinkedList.h"
#include "MatchingEngine.h"
#include "Sorting.h"
#include "Utils.h"

class LostFoundSystem {
    ItemLinkedList list_;
    std::unordered_map<std::string, Item*> idIndex_;  // hash table: ID -> item (list owns the item)
    MatchingEngine engine_;
    FileManager files_;
    std::function<void(Item*)> onItemAdded_;
    std::vector<std::pair<std::string, std::function<void()>>> extraMenu_;

public:
    explicit LostFoundSystem(const std::string& file = "data/items.txt") : files_(file) {}

    // ---------- plug-in points ----------
    void addMenuOption(const std::string& label, std::function<void()> fn) {
        extraMenu_.emplace_back(label, std::move(fn));
    }
    void setOnItemAdded(std::function<void(Item*)> fn) { onItemAdded_ = std::move(fn); }

    // ---------- data access ----------
    const ItemLinkedList& items() const { return list_; }
    Item* findById(const std::string& id) const {
        auto it = idIndex_.find(id);
        return it == idIndex_.end() ? nullptr : it->second;
    }

    // ---------- persistence ----------
    void load() {
        int n = files_.load(list_);
        for (const ListNode* p = list_.head(); p; p = p->next) idIndex_[p->item->id()] = p->item;
        std::cout << "Loaded " << n << " item record(s).\n";
    }
    void save() const {
        if (!files_.save(list_)) std::cout << "Warning: could not save items file.\n";
    }

    // ---------- features ----------
    // Interactive. Returns the new item, or nullptr if rejected.
    Item* addItem(bool isLost) {
        std::cout << "\n--- Report a " << (isLost ? "LOST" : "FOUND") << " item ---\n";
        std::string id = util::readLine("Item ID (unique, e.g. " + std::string(isLost ? "L001" : "F001") + "): ");
        if (id.empty()) { std::cout << "Rejected: ID cannot be empty.\n"; return nullptr; }
        if (findById(id)) { std::cout << "Rejected: ID '" << id << "' already exists.\n"; return nullptr; }

        std::string name = util::readNonEmpty("Item name (e.g. Black Backpack): ");
        std::string category = util::readNonEmpty("Category (e.g. bag, ID card, key, phone): ");
        std::string color = util::readNonEmpty("Colour: ");
        std::string location = util::readNonEmpty(isLost ? "Where did you lose it: " : "Where did you find it: ");
        std::string desc = util::readNonEmpty("Description: ");
        std::string who = util::readNonEmpty(isLost ? "Owner name: " : "Finder name: ");

        Item* it = isLost ? static_cast<Item*>(new LostItem(id, name, category, color, location, desc, who))
                          : static_cast<Item*>(new FoundItem(id, name, category, color, location, desc, who));
        list_.add(it);
        idIndex_[id] = it;
        if (onItemAdded_) onItemAdded_(it);  // extra modules can ask more questions here
        save();
        std::cout << "Report saved. Priority: " << priorityToString(it->priority()) << "\n";
        return it;
    }

    void searchById() const {
        std::string id = util::readNonEmpty("Enter item ID: ");
        Item* it = findById(id);
        if (it) it->displayPublic();
        else std::cout << "No item with ID '" << id << "'.\n";
    }

    // Priority queue: High first, then Medium, then Low (ties by ID).
    void priorityView() const {
        struct Cmp {
            bool operator()(const Item* a, const Item* b) const {
                if (a->priority() != b->priority()) return a->priority() > b->priority();
                return a->id() > b->id();
            }
        };
        std::priority_queue<const Item*, std::vector<const Item*>, Cmp> pq;
        for (const ListNode* p = list_.head(); p; p = p->next) pq.push(p->item);
        if (pq.empty()) { std::cout << "No items yet.\n"; return; }
        std::cout << "\n--- Items by priority ---\n";
        while (!pq.empty()) { pq.top()->displayPublic(); pq.pop(); }
    }

    void suggestMatches() const {
        std::string id = util::readNonEmpty("Enter your LOST item ID: ");
        const LostItem* lost = dynamic_cast<const LostItem*>(findById(id));
        if (!lost) { std::cout << "No LOST item with ID '" << id << "'.\n"; return; }
        auto results = engine_.suggest(*lost, list_);
        if (results.empty()) { std::cout << "No matching found items yet.\n"; return; }
        std::cout << "\n--- Suggested matches ---\n";
        for (const auto& r : results) {
            std::cout << "Match score: " << static_cast<int>(r.score + 0.5) << "%";
            if (!r.note.empty()) std::cout << "  (" << r.note << ")";
            std::cout << "\n";
            r.found->displayPublic();
        }
    }

    void listAll() const {
        if (list_.size() == 0) { std::cout << "No items yet.\n"; return; }
        std::cout << "\n--- All items (" << list_.size() << ") ---\n";
        for (const ListNode* p = list_.head(); p; p = p->next) p->item->displayPublic();
    }

    // Sorted views: merge sort (stable) for priority, quick sort for name / ID,
    // and binary search, which needs the list to be sorted by ID first.
    void sortedView() const {
        std::vector<Item*> v;
        for (const ListNode* p = list_.head(); p; p = p->next) v.push_back(p->item);
        if (v.empty()) { std::cout << "No items yet.\n"; return; }

        std::cout << "\n 1. By priority (merge sort)\n 2. By name A-Z (quick sort)\n"
                  << " 3. By ID (quick sort)\n 4. Binary search an item by ID\n";
        int k = util::readInt("Choice: ", 1, 4);

        if (k == 1)
            sorting::mergeSort(v, [](const Item* a, const Item* b) { return a->priority() < b->priority(); });
        else if (k == 2)
            sorting::quickSort(v, [](const Item* a, const Item* b) { return util::lower(a->name()) < util::lower(b->name()); });
        else  // 3 and 4 both need the list sorted by ID
            sorting::quickSort(v, [](const Item* a, const Item* b) { return a->id() < b->id(); });

        if (k == 4) {
            std::string id = util::readNonEmpty("Item ID to find: ");
            int steps = 0;
            int pos = sorting::binarySearch(v, id, [](const Item* a) { return a->id(); }, &steps);
            if (pos < 0) {
                std::cout << "Not found (" << steps << " comparisons for " << v.size() << " items).\n";
            } else {
                std::cout << "Found at position " << pos + 1 << " of " << v.size() << " after "
                          << steps << " comparison(s).\n";
                v[pos]->displayPublic();
            }
            return;
        }
        for (const Item* it : v)
            std::cout << " " << it->id() << " | " << it->type() << " | " << priorityToString(it->priority())
                      << " | " << it->name() << " (" << it->category() << ")\n";
    }

    // ---------- menu ----------
    void run() {
        const int coreOptions = 7;
        while (std::cin) {
            std::cout << "\n===== DSLib : Smart Hostel Lost & Found =====\n"
                      << " 1. Report a LOST item\n"
                      << " 2. Report a FOUND item\n"
                      << " 3. Search item by ID\n"
                      << " 4. View items by priority\n"
                      << " 5. Suggest matches for a lost item\n"
                      << " 6. List all items\n"
                      << " 7. Sorted views / binary search by ID\n";
            for (size_t i = 0; i < extraMenu_.size(); ++i)
                std::cout << " " << (coreOptions + 1 + i) << ". " << extraMenu_[i].first << "\n";
            std::cout << " 0. Save and exit\n";

            int choice = util::readInt("Choice: ", 0, coreOptions + static_cast<int>(extraMenu_.size()));
            if (choice == 0) break;
            switch (choice) {
                case 1: addItem(true); break;
                case 2: addItem(false); break;
                case 3: searchById(); break;
                case 4: priorityView(); break;
                case 5: suggestMatches(); break;
                case 6: listAll(); break;
                case 7: sortedView(); break;
                default: extraMenu_[choice - coreOptions - 1].second(); break;
            }
        }
        save();
        std::cout << "Saved. Goodbye!\n";
    }
};
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "ItemLinkedList.h"
#include "Levenshtein.h"

struct MatchResult {
    const FoundItem* found;
    double score;      
    std::string note;  
};

class MatchingEngine {
public:
    static constexpr double FIELD_THRESHOLD = 0.60;
    static constexpr double OVERALL_THRESHOLD = 45.0;

    double score(const LostItem& lost, const FoundItem& found) const {
        const double sims[5] = {
            lev::fieldSimilarity(lost.category(), found.category()),
            lev::fieldSimilarity(lost.name(), found.name()),
            lev::fieldSimilarity(lost.color(), found.color()),
            lev::fieldSimilarity(lost.location(), found.location()),
            lev::fieldSimilarity(lost.description(), found.description())};
        double total = 0.0;
        for (double s : sims)
            if (s >= FIELD_THRESHOLD) 
                total += s;
        return total / 5.0 * 100.0;
    }

    bool privateDetailsAgree(const LostItem& lost, const FoundItem& found) const {
        if (lost.privateDetail().empty() || found.privateDetail().empty()) 
            return false;
        return lev::fieldSimilarity(lost.privateDetail(), found.privateDetail()) >= FIELD_THRESHOLD;
    }

    std::vector<MatchResult> suggest(const LostItem& lost, const ItemLinkedList& list) const {
        std::vector<MatchResult> results;
        for (const ListNode* p = list.head(); p; p = p->next) {
            const FoundItem* f = dynamic_cast<const FoundItem*>(p->item);
            if (!f || f->status() != "OPEN") 
                continue;  
            double s = score(lost, *f);
            if (s < OVERALL_THRESHOLD) 
                continue;
            results.push_back({f, s, privateDetailsAgree(lost, *f) ? "private details agree" : ""});
        }
        std::sort(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
            if (a.score != b.score) 
                return a.score > b.score;
            return a.note.size() > b.note.size(); 
        });
        return results;
    }
};

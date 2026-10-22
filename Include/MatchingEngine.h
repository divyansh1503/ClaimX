#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "ItemLinkedList.h"

struct MatchResult {
    const FoundItem* found;
    double score;      
    std::string note;  
};

class MatchingEngine {
    static bool same(const std::string& a, const std::string& b) {
        std::string x = util::normalize(a), y = util::normalize(b);
        return !x.empty() && x == y;
    }

public:
    static constexpr double OVERALL_THRESHOLD = 45.0;

    double score(const LostItem& lost, const FoundItem& found) const {
        int hits = 0;
        if (same(lost.category(), found.category())) ++hits;
        if (same(lost.name(), found.name())) ++hits;
        if (same(lost.color(), found.color())) ++hits;
        if (same(lost.location(), found.location())) ++hits;
        if (same(lost.description(), found.description())) ++hits;
        return hits / 5.0 * 100.0;
    }

    std::vector<MatchResult> suggest(const LostItem& lost, const ItemLinkedList& list) const {
        std::vector<MatchResult> results;
        for (const ListNode* p = list.head(); p; p = p->next) {
            const FoundItem* f = dynamic_cast<const FoundItem*>(p->item);
            if (!f) continue;
            double s = score(lost, *f);
            if (s >= OVERALL_THRESHOLD) results.push_back({f, s, ""});
        }
        std::sort(results.begin(), results.end(),
                  [](const MatchResult& a, const MatchResult& b) { return a.score > b.score; });
        return results;
    }
};

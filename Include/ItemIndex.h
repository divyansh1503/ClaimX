#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "Item.h"
#include "hashindex.h"

class ItemIndex {
    HashIndex* byCategory_;
    HashIndex* byLocation_;

    static std::string key(const std::string& s) { return util::lower(util::trim(s)); }
    static std::vector<std::string> fetch(const HashIndex* h, const std::string& k) {
        int n = 0;
        const char* const* ids = hi_get(h, key(k).c_str(), &n);
        std::vector<std::string> out;
        for (int i = 0; i < n; ++i) out.emplace_back(ids[i]);
        return out;
    }
    static void collectCb(const char* k, int count, void* ctx) {
        static_cast<std::vector<std::pair<std::string, int>>*>(ctx)->emplace_back(k, count);
    }

public:
    ItemIndex() : byCategory_(hi_create(31)), byLocation_(hi_create(31)) {}
    ItemIndex(const ItemIndex&) = delete;
    ItemIndex& operator=(const ItemIndex&) = delete;
    ~ItemIndex() { hi_destroy(byCategory_); hi_destroy(byLocation_); }

    void add(const Item& it) {  
        hi_add(byCategory_, key(it.category()).c_str(), it.id().c_str());
        hi_add(byLocation_, key(it.location()).c_str(), it.id().c_str());
    }
    std::vector<std::string> byCategory(const std::string& c) const { return fetch(byCategory_, c); }
    std::vector<std::string> byLocation(const std::string& l) const { return fetch(byLocation_, l); }

    // (category, number of items), unsorted
    std::vector<std::pair<std::string, int>> categoryCounts() const {
        std::vector<std::pair<std::string, int>> out;
        hi_foreach_key(byCategory_, collectCb, &out);
        return out;
    }
    std::vector<std::pair<std::string, int>> locationCounts() const {
        std::vector<std::pair<std::string, int>> out;
        hi_foreach_key(byLocation_, collectCb, &out);
        return out;
    }
};

#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include "ItemLinkedList.h"

class FileManager {
    std::string path_;

public:
    explicit FileManager(std::string path = "data/items.txt") : path_(std::move(path)) {}

    bool save(const ItemLinkedList& list) const {
        std::filesystem::path p(path_);
        if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
        std::ofstream out(path_, std::ios::trunc);
        if (!out) return false;
        for (const ListNode* n = list.head(); n; n = n->next)
            out << n->item->serialize() << "\n";
        return true;
    }

    // Returns how many records were loaded (missing file = 0).
    int load(ItemLinkedList& list) const {
        std::ifstream in(path_);
        if (!in) return 0;
        int count = 0;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            Item* it = makeItemFromFields(util::split(line, '\t'));
            if (it) { list.add(it); ++count; }
        }
        return count;
    }
};
```
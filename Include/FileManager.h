#pragma once
// Saves / loads all records to one text file (tab separated, one item per line).
#include <fstream>
#include <string>
#include<filesystem>
#include "ItemLinkedList.h"

class FileManager {
    std::string path_;

public:
    explicit FileManager(std::string path = "data/items.txt") : path_(std::move(path)) {}

    bool save(const ItemLinkedList& list) const {
        util::ensureParentDir(path_);
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
#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

class Trie {
    struct Node {
        std::map<char, Node*> next;
        std::vector<std::string> ids;  
    };
    Node* root_;

    static void destroy(Node* n) {
        for (auto& kv : n->next) destroy(kv.second);
            delete n;
    }
    static void collect(const Node* n, std::vector<std::string>& out) {
        for (const auto& id : n->ids)
            if (std::find(out.begin(), out.end(), id) == out.end()) 
                out.push_back(id);
        for (const auto& kv : n->next) 
            collect(kv.second, out);
    }
    static std::string clean(const std::string& s) {
        std::string o;
        for (char c : s)
            if (std::isalnum(static_cast<unsigned char>(c)))
                o += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return o;
    }

public:
    Trie() : root_(new Node) {}
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;
    ~Trie() { 
        destroy(root_); 
    }

    void insert(const std::string& word, const std::string& id) { 
        std::string w = clean(word);
        if (w.empty()) 
            return;
        Node* cur = root_;
        for (char c : w) {
            Node*& child = cur->next[c];
            if (!child) 
                child = new Node;
            cur = child;
        }
        if (std::find(cur->ids.begin(), cur->ids.end(), id) == cur->ids.end()) 
            cur->ids.push_back(id);
    }

    void insertText(const std::string& text, const std::string& id) {
        std::string word;
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) 
                word += c;
            else { insert(word, id); word.clear(); }
        }
        insert(word, id);
    }

    std::vector<std::string> searchPrefix(const std::string& prefix) const {
        std::string p = clean(prefix);
        std::vector<std::string> out;
        if (p.empty()) return out;
        const Node* cur = root_;
        for (char c : p) {
            auto it = cur->next.find(c);
            if (it == cur->next.end()) 
                return out;
            cur = it->second;
        }
        collect(cur, out);
        std::sort(out.begin(), out.end());
        return out;
    }
};

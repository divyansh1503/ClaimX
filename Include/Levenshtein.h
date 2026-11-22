#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include "Utils.h"

namespace lev {

inline int distance(const std::string& a, const std::string& b) {
    std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= a.size(); ++i) {
        cur[0] = static_cast<int>(i);
        for (size_t j = 1; j <= b.size(); ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

inline double similarity(const std::string& a, const std::string& b) {
    std::string x = util::normalize(a), y = util::normalize(b);
    if (x.empty() || y.empty()) return 0.0;
    size_t longer = std::max(x.size(), y.size());
    return 1.0 - static_cast<double>(distance(x, y)) / static_cast<double>(longer);
}

inline std::vector<std::string> tokens(const std::string& text) {
    static const char* stop[] = {"the", "and", "with", "has", "have", "for", "its", "was", "are"};
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&]() {
        if (cur.size() > 3 && cur.back() == 's' && cur[cur.size() - 2] != 's') cur.pop_back();
        bool skip = cur.size() < 3;
        for (const char* s : stop) if (cur == s) skip = true;
        if (!skip) out.push_back(cur);
        cur.clear();
    };
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c)))
            cur += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else if (!cur.empty()) flush();
    }
    if (!cur.empty()) flush();
    return out;
}

inline double directional(const std::vector<std::string>& A, const std::vector<std::string>& B) {
    if (A.empty() || B.empty()) return 0.0;
    double total = 0.0;
    for (const auto& a : A) {
        double best = 0.0;
        for (const auto& b : B) {
            double s = 1.0 - static_cast<double>(distance(a, b)) /
                                 static_cast<double>(std::max(a.size(), b.size()));
            best = std::max(best, s);
        }
        if (best >= 0.75) total += best; 
    }
    return total / static_cast<double>(A.size());
}

inline double tokenSimilarity(const std::string& a, const std::string& b) {
    auto A = tokens(a), B = tokens(b);
    return (directional(A, B) + directional(B, A)) / 2.0;
}

inline double fieldSimilarity(const std::string& a, const std::string& b) {
    return std::max(similarity(a, b), tokenSimilarity(a, b));
}

}  

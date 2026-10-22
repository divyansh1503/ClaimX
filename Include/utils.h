#pragma once
// Small helpers shared by every module (string cleanup + safe console input).
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>   // mkdir
#ifdef _WIN32
#include <direct.h>     // _mkdir on Windows
#endif

namespace util {

inline void makeDirectory(const std::string& dir) {
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0755);
#endif
}

inline void ensureParentDir(const std::string& file) {
    size_t pos = file.find_last_of("/\\");
    if (pos != std::string::npos) makeDirectory(file.substr(0, pos));
}

inline std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Keeps only letters/digits in lower case: "Black  Bag" -> "blackbag".
// Makes comparison ignore case, spacing and punctuation.
inline std::string normalize(const std::string& s) {
    std::string out;
    for (char c : s)
        if (std::isalnum(static_cast<unsigned char>(c)))
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// Splits on a delimiter and KEEPS empty fields (needed for the tab-separated file).
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == delim) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

// Reads one line. Tabs are replaced by spaces because '\t' is our file separator.
inline std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    for (char& c : s) if (c == '\t') c = ' ';
    return trim(s);
}

inline std::string readNonEmpty(const std::string& prompt) {
    while (std::cin) {
        std::string s = readLine(prompt);
        if (!s.empty()) return s;
        std::cout << "  Input cannot be empty.\n";
    }
    return "";
}

// Reads an integer in [lo, hi]. If input ends (EOF) it returns lo.
inline int readInt(const std::string& prompt, int lo, int hi) {
    while (std::cin) {
        std::string s = readLine(prompt);
        try {
            size_t pos = 0;
            int v = std::stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {}
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
    return lo;
}

}  // namespace util
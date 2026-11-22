#pragma once

#include <string>
#include <vector>
#include "Utils.h"

enum class ClaimStatus { PENDING, FLAGGED, APPROVED, REJECTED };

inline std::string claimStatusToString(ClaimStatus s) {
    switch (s) {
        case ClaimStatus::PENDING:  return "PENDING";   
        case ClaimStatus::FLAGGED:  return "FLAGGED";   
        case ClaimStatus::APPROVED: return "APPROVED";
        default:                    return "REJECTED";
    }
}

inline ClaimStatus claimStatusFromString(const std::string& s) {
    if (s == "PENDING") return ClaimStatus::PENDING;
    if (s == "FLAGGED") return ClaimStatus::FLAGGED;
    if (s == "APPROVED") return ClaimStatus::APPROVED;
    return ClaimStatus::REJECTED;
}

struct Claim {
    std::string id;       
    std::string itemId;  
    std::string claimant;
    std::string answer;   
    ClaimStatus status = ClaimStatus::PENDING;
    int similarityPct = 0;  
    int seq = 0;           
    std::string adminNote;

    bool isOpen() const { return status == ClaimStatus::PENDING || status == ClaimStatus::FLAGGED; }

    std::string serialize() const {
        return id + "\t" + itemId + "\t" + claimant + "\t" + answer + "\t" + claimStatusToString(status) +
               "\t" + std::to_string(similarityPct) + "\t" + std::to_string(seq) + "\t" + adminNote;
    }

    static bool parse(const std::string& line, Claim& c) {
        auto f = util::split(line, '\t');
        if (f.size() < 7) return false;
        try {
            c.id = f[0]; c.itemId = f[1]; c.claimant = f[2]; c.answer = f[3];
            c.status = claimStatusFromString(f[4]);
            c.similarityPct = std::stoi(f[5]);
            c.seq = std::stoi(f[6]);
            c.adminNote = f.size() > 7 ? f[7] : "";
        } catch (...) { return false; }
        return true;
    }
};

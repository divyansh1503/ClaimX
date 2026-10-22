#pragma once
// Item (abstract base) + LostItem + FoundItem, so both kinds of record
// can be handled through one pointer type (Item*).
#include <iostream>
#include <string>
#include <vector>
#include "Utils.h"

enum class Priority { HIGH = 0, MEDIUM = 1, LOW = 2 };

inline std::string priorityToString(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "High";
        case Priority::MEDIUM: return "Medium";
        default:               return "Low";
    }
}

inline Priority priorityFromString(const std::string& s) {
    std::string l = util::lower(s);
    if (l == "high") return Priority::HIGH;
    if (l == "medium") return Priority::MEDIUM;
    return Priority::LOW;
}

// ID cards and keys are verified first.
inline Priority priorityForCategory(const std::string& category) {
    std::string c = util::normalize(category);
    if (c == "idcard" || c == "idcards" || c == "id" || c == "key" || c == "keys")
        return Priority::HIGH;
    if (c == "wallet" || c == "phone" || c == "laptop" || c == "charger" ||
        c == "electronics" || c == "earphones" || c == "headphones")
        return Priority::MEDIUM;
    return Priority::LOW;
}

class Item {
protected:
    std::string id_, name_, category_, color_, location_, description_, reporter_;
    Priority priority_;

public:
    Item(std::string id, std::string name, std::string category, std::string color,
         std::string location, std::string description, std::string reporter)
        : id_(std::move(id)), name_(std::move(name)), category_(std::move(category)),
          color_(std::move(color)), location_(std::move(location)),
          description_(std::move(description)), reporter_(std::move(reporter)),
          priority_(priorityForCategory(category_)) {}

    virtual ~Item() = default;

    virtual std::string type() const = 0;           // "LOST" or "FOUND"
    virtual std::string reporterLabel() const = 0;  // "Owner" or "Finder"

    // FULL details - meant for the admin only.
    virtual void display() const {
        std::cout << "[" << type() << "] ID: " << id_
                  << " | Priority: " << priorityToString(priority_) << "\n"
                  << "   Item: " << name_ << " (" << category_ << ")\n"
                  << "   Colour: " << color_ << " | Location: " << location_ << "\n"
                  << "   Description: " << description_ << "\n"
                  << "   " << reporterLabel() << ": " << reporter_ << "\n";
    }

    // What ANY student may see: no colour, no description, no reporter name.
    // (Those are exactly the things someone would need to fake a claim.)
    virtual void displayPublic() const {
        std::cout << "[" << type() << "] ID: " << id_
                  << " | Priority: " << priorityToString(priority_) << "\n"
                  << "   Item: " << name_ << " (" << category_ << ")\n"
                  << "   Location: " << location_ << "\n"
                  << "   (colour, description and reporter name are hidden)\n";
    }

    // One tab-separated line for items.txt
    virtual std::string serialize() const {
        return type() + "\t" + id_ + "\t" + name_ + "\t" + category_ + "\t" + color_ + "\t" +
               location_ + "\t" + description_ + "\t" + priorityToString(priority_) + "\t" +
               reporter_;
    }

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& category() const { return category_; }
    const std::string& color() const { return color_; }
    const std::string& location() const { return location_; }
    const std::string& description() const { return description_; }
    const std::string& reporter() const { return reporter_; }
    Priority priority() const { return priority_; }
    void setPriority(Priority p) { priority_ = p; }
};

class LostItem : public Item {
public:
    using Item::Item;
    std::string type() const override { return "LOST"; }
    std::string reporterLabel() const override { return "Owner"; }
};

class FoundItem : public Item {
public:
    using Item::Item;
    std::string type() const override { return "FOUND"; }
    std::string reporterLabel() const override { return "Finder"; }
};

// Builds an item from one line of items.txt (already split on '\t').
// Returns nullptr if the line is malformed.
inline Item* makeItemFromFields(const std::vector<std::string>& f) {
    if (f.size() < 9) return nullptr;
    Item* it = nullptr;
    if (f[0] == "LOST")       it = new LostItem(f[1], f[2], f[3], f[4], f[5], f[6], f[8]);
    else if (f[0] == "FOUND") it = new FoundItem(f[1], f[2], f[3], f[4], f[5], f[6], f[8]);
    else return nullptr;
    it->setPriority(priorityFromString(f[7]));
    return it;
}
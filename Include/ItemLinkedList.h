
#pragma once
// Our own singly linked list. It OWNS the items (deletes them in the destructor).
// A tail pointer makes add() O(1) instead of walking to the end every time.
#include <string>
#include "Item.h"

struct ListNode {
    Item* item;
    ListNode* next;
};

class ItemLinkedList {
    ListNode* head_ = nullptr;
    ListNode* tail_ = nullptr;
    size_t size_ = 0;

public:
    ItemLinkedList() = default;
    ItemLinkedList(const ItemLinkedList&) = delete;
    ItemLinkedList& operator=(const ItemLinkedList&) = delete;
    ~ItemLinkedList() { clear(); }

    void add(Item* item) {  // takes ownership
        ListNode* node = new ListNode{item, nullptr};
        if (!head_) head_ = tail_ = node;
        else { tail_->next = node; tail_ = node; }
        ++size_;
    }

    Item* find(const std::string& id) const {  // O(n)
        for (ListNode* p = head_; p; p = p->next)
            if (p->item->id() == id) return p->item;
        return nullptr;
    }

    ListNode* head() const { return head_; }
    size_t size() const { return size_; }

    void clear() {
        while (head_) {
            ListNode* next = head_->next;
            delete head_->item;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }
};

#ifndef DOUBLY_LINKED_LIST_HPP
#define DOUBLY_LINKED_LIST_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace vmem {

template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* prev;
        Node* next;

        Node(const T& val) : data(val), prev(nullptr), next(nullptr) {}
        Node(T&& val) : data(std::move(val)), prev(nullptr), next(nullptr) {}
    };

private:
    Node* head;
    Node* tail;
    size_t count;

public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

    ~DoublyLinkedList() {
        clear();
    }

    // Disable copy for simplicity in resource management, enable move
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : head(other.head), tail(other.tail), count(other.count) {
        other.head = nullptr;
        other.tail = nullptr;
        other.count = 0;
    }

    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head = other.head;
            tail = other.tail;
            count = other.count;
            other.head = nullptr;
            other.tail = nullptr;
            other.count = 0;
        }
        return *this;
    }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }

    Node* getHead() const { return head; }
    Node* getTail() const { return tail; }

    Node* push_front(const T& value) {
        Node* newNode = new Node(value);
        if (!head) {
            head = tail = newNode;
        } else {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        }
        ++count;
        return newNode;
    }

    Node* push_back(const T& value) {
        Node* newNode = new Node(value);
        if (!tail) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        ++count;
        return newNode;
    }

    void remove(Node* node) {
        if (!node) return;

        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head = node->next;
        }

        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail = node->prev;
        }

        delete node;
        --count;
    }

    // Unlink node from list without deallocating memory
    void detach(Node* node) {
        if (!node) return;

        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head = node->next;
        }

        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail = node->prev;
        }

        node->prev = nullptr;
        node->next = nullptr;
        --count;
    }

    // Attach an existing detached node to the front (O(1))
    void attach_front(Node* node) {
        if (!node) return;
        node->prev = nullptr;
        node->next = head;
        if (!head) {
            head = tail = node;
        } else {
            head->prev = node;
            head = node;
        }
        ++count;
    }

    // Move an existing node in the list to the front (O(1))
    void move_to_front(Node* node) {
        if (!node || node == head) return;
        detach(node);
        attach_front(node);
    }

    // Move an existing node in the list to the back (O(1))
    void move_to_back(Node* node) {
        if (!node || node == tail) return;
        detach(node);
        node->next = nullptr;
        node->prev = tail;
        if (!tail) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ++count;
    }

    T pop_back() {
        if (empty()) {
            throw std::runtime_error("List is empty");
        }
        Node* victim = tail;
        T val = victim->data;
        remove(victim);
        return val;
    }

    T pop_front() {
        if (empty()) {
            throw std::runtime_error("List is empty");
        }
        Node* victim = head;
        T val = victim->data;
        remove(victim);
        return val;
    }

    void clear() {
        Node* curr = head;
        while (curr) {
            Node* nxt = curr->next;
            delete curr;
            curr = nxt;
        }
        head = tail = nullptr;
        count = 0;
    }
};

} // namespace vmem

#endif // DOUBLY_LINKED_LIST_HPP

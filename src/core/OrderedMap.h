#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <unordered_map>

// JavaScript Object.keys order: array-index keys first; other string keys in insertion order.
// Stable nodes keep references valid. Indexes hold raw node pointers: retaining
// std::list checked iterators makes MSVC Debug teardown and erase quadratic.
template <class T> class OrderedMap {
  public:
    using value_type = std::pair<const std::string, T>;

  private:
    struct Link { Link* previous = this; Link* next = this; };
    struct Node : Link {
        value_type value;
        explicit Node(const std::string& key) : value(key, T{}) {}
    };

  public:
    template<bool Constant> class Iterator {
        using LinkType = std::conditional_t<Constant, const Link, Link>;
        using NodeType = std::conditional_t<Constant, const Node, Node>;
        LinkType* link_ = nullptr;
        explicit Iterator(LinkType* link) : link_(link) {}
        friend class OrderedMap;
        template<bool> friend class Iterator;
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename OrderedMap::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<Constant, const value_type*, value_type*>;
        using reference = std::conditional_t<Constant, const value_type&, value_type&>;
        Iterator() = default;
        template<bool Other, std::enable_if_t<Constant && !Other, int> = 0>
        Iterator(const Iterator<Other>& other) : link_(other.link_) {}
        reference operator*() const { return static_cast<NodeType*>(link_)->value; }
        pointer operator->() const { return &**this; }
        Iterator& operator++() { link_ = link_->next; return *this; }
        Iterator operator++(int) { auto previous = *this; ++*this; return previous; }
        Iterator& operator--() { link_ = link_->previous; return *this; }
        Iterator operator--(int) { auto previous = *this; --*this; return previous; }
        template<bool Other> bool operator==(const Iterator<Other>& other) const { return link_ == other.link_; }
    };
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    OrderedMap() = default;
    OrderedMap(std::initializer_list<value_type> entries) {
        for (const auto& [key, value] : entries)
            (*this)[key] = value;
    }
    OrderedMap(const OrderedMap& other) {
        for (const auto& [key, value] : other) (*this)[key] = value;
    }
    ~OrderedMap() { clear(); }
    OrderedMap& operator=(const OrderedMap& other) {
        if (this != &other) {
            OrderedMap copied(other);
            swap(copied);
        }
        return *this;
    }
    OrderedMap(OrderedMap&& other) { swap(other); }
    OrderedMap& operator=(OrderedMap&& other) noexcept {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }
    void swap(OrderedMap& other) noexcept {
        if (this == &other) return;
        const bool wasEmpty = empty(), otherWasEmpty = other.empty();
        lookup_.swap(other.lookup_);
        numeric_.swap(other.numeric_);
        std::swap(root_.next, other.root_.next);
        std::swap(root_.previous, other.root_.previous);
        if (otherWasEmpty) root_.next = root_.previous = &root_;
        else root_.next->previous = root_.previous->next = &root_;
        if (wasEmpty) other.root_.next = other.root_.previous = &other.root_;
        else other.root_.next->previous = other.root_.previous->next = &other.root_;
    }
    iterator begin() { return iterator(root_.next); }
    iterator end() { return iterator(&root_); }
    const_iterator begin() const { return const_iterator(root_.next); }
    const_iterator end() const { return const_iterator(&root_); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    bool empty() const { return lookup_.empty(); }
    std::size_t size() const { return lookup_.size(); }
    void clear() {
        numeric_.clear();
        while (root_.next != &root_) {
            auto* node = static_cast<Node*>(root_.next);
            const auto found = lookup_.find(node->value.first);
            unlink(node);
            lookup_.erase(found);
        }
    }
    iterator find(const std::string& key) {
        const auto entry = lookup_.find(key);
        return entry == lookup_.end() ? end() : iterator(entry->second.get());
    }
    const_iterator find(const std::string& key) const {
        const auto entry = lookup_.find(key);
        return entry == lookup_.end() ? end() : const_iterator(entry->second.get());
    }
    bool contains(const std::string& key) const { return find(key) != end(); }
    template <class U> std::pair<iterator, bool> emplace(const std::string& key, U&& value) {
        auto existing = find(key);
        if (existing != end()) return {existing, false};
        (*this)[key] = std::forward<U>(value);
        return {find(key), true};
    }
    bool operator==(const OrderedMap& other) const {
        if (size() != other.size()) return false;
        for (const auto& [key, value] : *this) {
            const auto entry = other.find(key);
            if (entry == other.end() || !(value == entry->second)) return false;
        }
        return true;
    }
    T& operator[](const std::string& key) {
        auto existing = find(key);
        if (existing != end())
            return existing->second;
        Link* position = &root_;
        const auto index = arrayIndex(key);
        if (index) {
            const auto next = numeric_.upper_bound(*index);
            position = next != numeric_.end() ? next->second
                : numeric_.empty() ? root_.next : numeric_.rbegin()->second->next;
        }
        auto owned = std::make_unique<Node>(key);
        auto* inserted = owned.get();
        const auto location = lookup_.emplace(key, std::move(owned)).first;
        try {
            if (index) numeric_.emplace(*index, inserted);
        } catch (...) {
            lookup_.erase(location);
            throw;
        }
        inserted->previous = position->previous;
        inserted->next = position;
        position->previous->next = inserted;
        position->previous = inserted;
        return inserted->value.second;
    }
    T& at(const std::string& key) {
        const auto entry = find(key);
        if (entry == end())
            throw std::out_of_range("OrderedMap::at");
        return entry->second;
    }
    const T& at(const std::string& key) const {
        const auto entry = find(key);
        if (entry == end())
            throw std::out_of_range("OrderedMap::at");
        return entry->second;
    }
    std::size_t erase(const std::string& key) {
        const auto entry = lookup_.find(key);
        if (entry == lookup_.end())
            return 0;
        if (const auto index = arrayIndex(key)) numeric_.erase(*index);
        unlink(entry->second.get());
        lookup_.erase(entry);
        return 1;
    }

  private:
    Link root_;
    std::unordered_map<std::string, std::unique_ptr<Node>> lookup_;
    std::map<std::uint32_t, Node*> numeric_;
    static void unlink(Link* node) {
        node->previous->next = node->next;
        node->next->previous = node->previous;
    }
    static std::optional<std::uint32_t> arrayIndex(const std::string& key) {
        if (key.empty() || (key.size() > 1 && key[0] == '0'))
            return std::nullopt;
        std::uint64_t value = 0;
        for (const char character : key) {
            if (character < '0' || character > '9')
                return std::nullopt;
            value = value * 10 + (character - '0');
            if (value >= 0xffffffffu)
                return std::nullopt;
        }
        return static_cast<std::uint32_t>(value);
    }
};

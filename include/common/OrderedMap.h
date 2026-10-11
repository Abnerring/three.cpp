#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <list>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// JavaScript Object.keys order: array-index keys first; other string keys in insertion order.
// List storage keeps references to existing values valid when other attributes are added.
template <class T> class OrderedMap {
  public:
    using value_type = std::pair<const std::string, T>;
    using Storage = std::list<value_type>;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;

    OrderedMap() = default;
    OrderedMap(std::initializer_list<value_type> entries) {
        for (const auto& [key, value] : entries)
            (*this)[key] = value;
    }
    OrderedMap(const OrderedMap&) = default;
    OrderedMap& operator=(const OrderedMap& other) {
        if (this != &other) {
            Storage copied(other.entries_);
            entries_.swap(copied);
        }
        return *this;
    }
    OrderedMap(OrderedMap&&) noexcept = default;
    OrderedMap& operator=(OrderedMap&&) noexcept = default;
    iterator begin() { return entries_.begin(); }
    iterator end() { return entries_.end(); }
    const_iterator begin() const { return entries_.begin(); }
    const_iterator end() const { return entries_.end(); }
    const_iterator cbegin() const { return entries_.cbegin(); }
    const_iterator cend() const { return entries_.cend(); }
    bool empty() const { return entries_.empty(); }
    std::size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }
    iterator find(const std::string& key) {
        return std::find_if(begin(), end(), [&](const auto& entry) { return entry.first == key; });
    }
    const_iterator find(const std::string& key) const {
        return std::find_if(begin(), end(), [&](const auto& entry) { return entry.first == key; });
    }
    bool contains(const std::string& key) const { return find(key) != end(); }
    T& operator[](const std::string& key) {
        auto existing = find(key);
        if (existing != end())
            return existing->second;
        auto position = end();
        if (const auto index = arrayIndex(key)) {
            position = std::find_if(begin(), end(), [&](const auto& entry) {
                const auto otherIndex = arrayIndex(entry.first);
                return !otherIndex || *otherIndex > *index;
            });
        }
        return entries_.emplace(position, key, T{})->second;
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
        const auto entry = find(key);
        if (entry == end())
            return 0;
        entries_.erase(entry);
        return 1;
    }

  private:
    Storage entries_;
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

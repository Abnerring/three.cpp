#pragma once
#include "core/OrderedMap.h"
#include <memory>
#include <utility>

// Assignment shares an object. Explicit construction creates a new map, as
// replacing a JavaScript field with a fresh object does.
template <class T> class SharedOrderedMap {
  public:
    using Storage = OrderedMap<T>;
    using value_type = typename Storage::value_type;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;

    SharedOrderedMap() : data_(std::make_shared<Storage>()) {}
    SharedOrderedMap(std::initializer_list<value_type> entries)
        : data_(std::make_shared<Storage>(entries)) {}
    explicit SharedOrderedMap(std::shared_ptr<Storage> storage) : data_(std::move(storage)) {}
    SharedOrderedMap& operator=(std::initializer_list<value_type> entries) {
        data_ = std::make_shared<Storage>(entries); return *this;
    }
    std::shared_ptr<Storage> storage() const { return data_; }
    bool sameIdentity(const SharedOrderedMap& other) const { return data_ == other.data_; }
    bool operator==(const SharedOrderedMap& other) const { return *data_ == *other.data_; }
    iterator begin() { return data_->begin(); }
    iterator end() { return data_->end(); }
    const_iterator begin() const { return data_->begin(); }
    const_iterator end() const { return data_->end(); }
    const_iterator cbegin() const { return data_->cbegin(); }
    const_iterator cend() const { return data_->cend(); }
    bool empty() const { return data_->empty(); }
    std::size_t size() const { return data_->size(); }
    void clear() { data_->clear(); }
    iterator find(const std::string& key) { return data_->find(key); }
    const_iterator find(const std::string& key) const { return data_->find(key); }
    bool contains(const std::string& key) const { return data_->contains(key); }
    T& operator[](const std::string& key) { return (*data_)[key]; }
    T& at(const std::string& key) { return data_->at(key); }
    const T& at(const std::string& key) const { return data_->at(key); }
    std::size_t erase(const std::string& key) { return data_->erase(key); }
    template <class U> std::pair<iterator, bool> emplace(const std::string& key, U&& value) {
        return data_->emplace(key, std::forward<U>(value));
    }

  private:
    std::shared_ptr<Storage> data_;
};

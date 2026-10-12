#pragma once
#include "core/JSValue.h"
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <vector>

// A native array handle. Assignment aliases storage; slice copies the container.
// Descriptor flags and holes belong to the array, not to a temporary JS wrapper.
template <class T> class SharedArray {
  public:
    struct Flags { bool writable = true, enumerable = true, configurable = true; };
    struct Storage {
        std::vector<T> values;
        std::set<std::size_t> holes;
        std::map<std::size_t, Flags> flags;
        bool lengthWritable = true;
    };
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    SharedArray() : data_(std::make_shared<Storage>()) {}
    SharedArray(std::vector<T> values) : SharedArray() { data_->values = std::move(values); }
    SharedArray(std::initializer_list<T> values) : SharedArray(std::vector<T>(values)) {}
    explicit SharedArray(std::shared_ptr<Storage> storage) : data_(std::move(storage)) {}
    SharedArray& operator=(std::vector<T> values) { *this = SharedArray(std::move(values)); return *this; }
    SharedArray& operator=(std::initializer_list<T> values) { *this = SharedArray(values); return *this; }
    std::shared_ptr<Storage> storage() const { return data_; }
    SharedArray slice() const {
        auto copy = std::make_shared<Storage>();
        copy->values = data_->values; copy->holes = data_->holes;
        return SharedArray(std::move(copy));
    }
    bool sameIdentity(const SharedArray& other) const { return data_ == other.data_; }
    bool operator==(const SharedArray& other) const { return data_->values == other.data_->values; }
    std::size_t size() const { return data_->values.size(); }
    bool empty() const { return data_->values.empty(); }
    T& operator[](std::size_t i) { return data_->values[i]; }
    const T& operator[](std::size_t i) const { return data_->values[i]; }
    T& at(std::size_t i) { return data_->values.at(i); }
    const T& at(std::size_t i) const { return data_->values.at(i); }
    T& front() { return data_->values.front(); }
    const T& front() const { return data_->values.front(); }
    T& back() { return data_->values.back(); }
    const T& back() const { return data_->values.back(); }
    iterator begin() { return data_->values.begin(); }
    iterator end() { return data_->values.end(); }
    const_iterator begin() const { return data_->values.begin(); }
    const_iterator end() const { return data_->values.end(); }
    void reserve(std::size_t n) { data_->values.reserve(n); }
    void push_back(const T& value) {
        if (!data_->lengthWritable) throw JSTypeError("Cannot assign to read only array length");
        data_->values.push_back(value);
    }
    void push_back(T&& value) {
        if (!data_->lengthWritable) throw JSTypeError("Cannot assign to read only array length");
        data_->values.push_back(std::move(value));
    }
    void clear() {
        if (!data_->lengthWritable) throw JSTypeError("Cannot assign to read only array length");
        for (std::size_t i = size(); i > 0; --i) {
            if (!data_->holes.contains(i - 1) && data_->flags.contains(i - 1) && !data_->flags.at(i - 1).configurable)
                throw JSTypeError("Cannot delete nonconfigurable array element");
            data_->values.pop_back(); data_->holes.erase(i - 1); data_->flags.erase(i - 1);
        }
    }
    iterator erase(iterator position) {
        const auto index = std::size_t(position - begin());
        const auto remove = [&](std::size_t i) {
            if (!data_->holes.contains(i) && data_->flags.contains(i) && !data_->flags.at(i).configurable)
                throw JSTypeError("Cannot delete nonconfigurable array element");
            data_->values[i] = T{}; data_->holes.insert(i); data_->flags.erase(i);
        };
        for (std::size_t i = index; i + 1 < size(); ++i) {
            if (data_->holes.contains(i + 1)) remove(i);
            else {
                if (!data_->holes.contains(i) && data_->flags.contains(i) && !data_->flags.at(i).writable)
                    throw JSTypeError("Cannot assign to read only array element");
                data_->values[i] = data_->values[i + 1];
                if (data_->holes.erase(i)) data_->flags.erase(i);
            }
        }
        remove(size() - 1);
        if (!data_->lengthWritable) throw JSTypeError("Cannot assign to read only array length");
        data_->holes.erase(size() - 1); data_->values.pop_back();
        return data_->values.begin() + index;
    }
  private:
    std::shared_ptr<Storage> data_;
};

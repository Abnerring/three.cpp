#pragma once

#include <algorithm>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// Elements have stable identities. Container copies share elements, like a JS array slice.
// Keep owner(element) when the element must survive erase/clear/replacement.
template <class T> class SharedVector {
    using Storage = std::vector<std::shared_ptr<T>>;

    template <bool Const> class Iterator {
        using Base =
            std::conditional_t<Const, typename Storage::const_iterator, typename Storage::iterator>;
        Base position_;
        explicit Iterator(Base position) : position_(position) {}
        friend class SharedVector;
        template <bool> friend class Iterator;

      public:
        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = typename Storage::difference_type;
        using reference = std::conditional_t<Const, const T&, T&>;
        using pointer = std::conditional_t<Const, const T*, T*>;
        Iterator() = default;
        template <bool Other>
            requires(Const && !Other)
        Iterator(const Iterator<Other>& other) : position_(other.position_) {}
        reference operator*() const { return **position_; }
        pointer operator->() const { return position_->get(); }
        reference operator[](difference_type offset) const { return *position_[offset]; }
        Iterator& operator++() {
            ++position_;
            return *this;
        }
        Iterator operator++(int) {
            auto old = *this;
            ++*this;
            return old;
        }
        Iterator& operator--() {
            --position_;
            return *this;
        }
        Iterator operator--(int) {
            auto old = *this;
            --*this;
            return old;
        }
        Iterator& operator+=(difference_type offset) {
            position_ += offset;
            return *this;
        }
        Iterator& operator-=(difference_type offset) {
            position_ -= offset;
            return *this;
        }
        friend Iterator operator+(Iterator position, difference_type offset) {
            position += offset;
            return position;
        }
        friend Iterator operator+(difference_type offset, Iterator position) {
            position += offset;
            return position;
        }
        friend Iterator operator-(Iterator position, difference_type offset) {
            position -= offset;
            return position;
        }
        friend difference_type operator-(const Iterator& left, const Iterator& right) {
            return left.position_ - right.position_;
        }
        auto operator<=>(const Iterator&) const = default;
    };

  public:
    using value_type = T;
    using size_type = typename Storage::size_type;
    using difference_type = typename Storage::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    SharedVector() = default;
    SharedVector(std::initializer_list<T> values) {
        reserve(values.size());
        for (const auto& value : values)
            push_back(value);
    }
    SharedVector(const SharedVector&) = default;
    SharedVector& operator=(const SharedVector&) = default;
    SharedVector(SharedVector&&) noexcept = default;
    SharedVector& operator=(SharedVector&&) noexcept = default;
    SharedVector& operator=(std::initializer_list<T> values) {
        SharedVector replacement(values);
        swap(replacement);
        return *this;
    }

    bool empty() const noexcept { return entries_.empty(); }
    size_type size() const noexcept { return entries_.size(); }
    size_type capacity() const noexcept { return entries_.capacity(); }
    void reserve(size_type count) { entries_.reserve(count); }
    void clear() noexcept { entries_.clear(); }
    T& operator[](size_type index) { return *entries_[index]; }
    const T& operator[](size_type index) const { return *entries_[index]; }
    T& at(size_type index) { return *entries_.at(index); }
    const T& at(size_type index) const { return *entries_.at(index); }
    T& front() { return *entries_.front(); }
    const T& front() const { return *entries_.front(); }
    T& back() { return *entries_.back(); }
    const T& back() const { return *entries_.back(); }
    iterator begin() noexcept { return iterator(entries_.begin()); }
    iterator end() noexcept { return iterator(entries_.end()); }
    const_iterator begin() const noexcept { return const_iterator(entries_.begin()); }
    const_iterator end() const noexcept { return const_iterator(entries_.end()); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    void push_back(const T& value) { entries_.push_back(std::make_shared<T>(value)); }
    void push_back(T&& value) { entries_.push_back(std::make_shared<T>(std::move(value))); }
    template <class... Args> T& emplace_back(Args&&... args) {
        entries_.push_back(std::make_shared<T>(std::forward<Args>(args)...));
        return *entries_.back();
    }
    void pop_back() { entries_.pop_back(); }
    iterator erase(const_iterator position) { return iterator(entries_.erase(position.position_)); }
    iterator erase(const_iterator first, const_iterator last) {
        return iterator(entries_.erase(first.position_, last.position_));
    }
    iterator insert(const_iterator position, const T& value) {
        return iterator(entries_.insert(position.position_, std::make_shared<T>(value)));
    }
    iterator insert(const_iterator position, T&& value) {
        return iterator(entries_.insert(position.position_, std::make_shared<T>(std::move(value))));
    }
    T& replace(size_type index, const T& value) {
        entries_.at(index) = std::make_shared<T>(value);
        return *entries_[index];
    }
    void swap(SharedVector& other) noexcept { entries_.swap(other.entries_); }
    std::shared_ptr<T> owner(const T& value) {
        const auto found = findOwner(value);
        return found == entries_.end() ? nullptr : *found;
    }
    std::shared_ptr<const T> owner(const T& value) const {
        const auto found = findOwner(value);
        return found == entries_.end() ? nullptr : *found;
    }

  private:
    Storage entries_;
    typename Storage::const_iterator findOwner(const T& value) const {
        return std::find_if(entries_.begin(), entries_.end(), [&](const auto& entry) {
            return entry.get() == std::addressof(value);
        });
    }
};

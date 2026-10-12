#pragma once

#include "core/JSValue.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

template <class T, bool Clamped = false> class TypedArray;

namespace TypedArray_detail {
inline std::size_t toIndex(double value) {
    if (std::isnan(value) || value == 0) return 0;
    value = std::trunc(value);
    if (!std::isfinite(value) || value < 0 || value > 9007199254740991.0 ||
        value > double(std::numeric_limits<std::size_t>::max()))
        throw JSRangeError("Invalid buffer index");
    return std::size_t(value);
}
}

// Numeric accessors keep JavaScript's read-only storage metadata visible as fields.
class ReadOnlySize {
  public:
    ReadOnlySize(const ReadOnlySize&) = default;
    operator std::size_t() const { return value_; }
  private:
    explicit ReadOnlySize(std::size_t value = 0) : value_(value) {}
    ReadOnlySize& operator=(const ReadOnlySize&) = default;
    std::size_t value_;
    friend class ArrayBuffer;
    template <class T, bool Clamped> friend class TypedArray;
};

class ArrayBuffer {
  public:
    ReadOnlySize byteLength;
    explicit ArrayBuffer(std::size_t length = 0)
        : byteLength(length), bytes_(std::make_shared<std::vector<std::byte>>(length)) {}
    template <class N, std::enable_if_t<std::is_arithmetic_v<N> && !std::is_same_v<N, std::size_t>, int> = 0>
    explicit ArrayBuffer(N length) : ArrayBuffer(TypedArray_detail::toIndex(double(length))) {}
    ArrayBuffer(const ArrayBuffer&) = default;
    ArrayBuffer& operator=(const ArrayBuffer&) = default;
    bool sameIdentity(const ArrayBuffer& other) const { return bytes_ == other.bytes_; }
    const void* identity() const { return bytes_.get(); }
    ArrayBuffer slice(double begin = 0, std::optional<double> end = std::nullopt) const {
        const auto first = relativeIndex(begin, *this);
        const auto last = end ? relativeIndex(*end, *this) : std::size_t(byteLength);
        ArrayBuffer result(last > first ? last - first : 0);
        if (result.byteLength) std::memcpy(result.bytes_->data(), bytes_->data() + first,
                                         std::size_t(result.byteLength));
        return result;
    }
  private:
    std::shared_ptr<std::vector<std::byte>> bytes_;
    explicit ArrayBuffer(std::shared_ptr<std::vector<std::byte>> bytes)
        : byteLength(bytes->size()), bytes_(std::move(bytes)) {}
    friend struct NativeArrayAccess;
    static std::size_t relativeIndex(double value, const ArrayBuffer& buffer) {
        return relativeIndex(value, std::size_t(buffer.byteLength));
    }
    static std::size_t relativeIndex(double value, std::size_t size) {
        if (std::isnan(value)) return 0;
        value = std::trunc(value);
        if (value < 0) return std::size_t(std::max(double(size) + value, 0.0));
        return std::size_t(std::min(value, double(size)));
    }
    template <class T, bool Clamped> friend class TypedArray;
};

// Copies of a handle preserve the TypedArray identity. subarray creates a new
// view on the same ArrayBuffer; slice and constructors from values copy bytes.
template <class T, bool Clamped> class TypedArray {
    static_assert((std::is_integral_v<T> && sizeof(T) <= 4 && !std::is_same_v<T, bool>) ||
                  std::is_same_v<T, float> || std::is_same_v<T, double>);
    static_assert(!Clamped || std::is_same_v<T, std::uint8_t>);
    struct View {
        ArrayBuffer buffer;
        std::size_t offset, count;
        View(ArrayBuffer storage, std::size_t start, std::size_t size)
            : buffer(std::move(storage)), offset(start), count(size) {}
    };
    ArrayBuffer buffer_;
  public:
    using value_type = T;
    static constexpr std::size_t BYTES_PER_ELEMENT = sizeof(T);
    static constexpr bool isClamped = Clamped;
    const ArrayBuffer& buffer;
    ReadOnlySize byteOffset, byteLength, length;

    explicit TypedArray(std::size_t count = 0)
        : TypedArray(ArrayBuffer(checkedBytes(count)), 0, double(count)) {}
    template <class N, std::enable_if_t<std::is_arithmetic_v<N> && !std::is_same_v<N, std::size_t>, int> = 0>
    explicit TypedArray(N count) : TypedArray(TypedArray_detail::toIndex(double(count))) {}
    explicit TypedArray(const std::vector<T>& values) : TypedArray(values.size()) {
        if (!values.empty()) std::memcpy(buffer.bytes_->data(), values.data(),
                                         std::size_t(byteLength));
    }
    template <class U, std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<T, U>, int> = 0>
    explicit TypedArray(const std::vector<U>& values) : TypedArray(values.size()) { set(values); }
    template <class U, bool C> requires (!std::is_same_v<T, U> || Clamped != C)
    explicit TypedArray(const TypedArray<U, C>& values) : TypedArray(values.size()) { set(values); }
    TypedArray(std::initializer_list<T> values) : TypedArray(std::vector<T>(values)) {}
    explicit TypedArray(ArrayBuffer storage, double requestedOffset = 0,
                        std::optional<double> count = std::nullopt)
        : buffer_(std::move(storage)), buffer(buffer_), byteOffset(0), byteLength(0), length(0) {
        const auto offset = TypedArray_detail::toIndex(requestedOffset);
        byteOffset = ReadOnlySize(offset);
        if (offset % sizeof(T) || offset > std::size_t(buffer.byteLength))
            throw JSRangeError("TypedArray byteOffset is outside the buffer or unaligned");
        const auto available = std::size_t(buffer.byteLength) - offset;
        if (!count && available % sizeof(T))
            throw JSRangeError("TypedArray buffer byteLength is unaligned");
        const auto size = count ? TypedArray_detail::toIndex(*count) : available / sizeof(T);
        if (size > available / sizeof(T)) throw JSRangeError("TypedArray view exceeds buffer");
        length = ReadOnlySize(size);
        byteLength = ReadOnlySize(checkedBytes(size));
        view_ = std::make_shared<View>(buffer, offset, size);
    }
    TypedArray(const TypedArray& other)
        : buffer_(other.buffer_), buffer(buffer_), byteOffset(other.byteOffset),
          byteLength(other.byteLength), length(other.length), view_(other.view_) {}
    TypedArray& operator=(const TypedArray& other) {
        buffer_ = other.buffer_;
        byteOffset = other.byteOffset;
        byteLength = other.byteLength;
        length = other.length;
        view_ = other.view_;
        return *this;
    }
    bool sameIdentity(const TypedArray& other) const { return view_ == other.view_; }
    const void* identity() const { return view_.get(); }
    std::size_t size() const { return length; }
    bool empty() const { return size() == 0; }

    class Reference {
      public:
        operator T() const { return loadView(*view_, index_); }
        const Reference& operator=(double value) const { storeView(*view_, index_, value); return *this; }
        const Reference& operator=(const Reference& other) const { return *this = double(T(other)); }
        friend void swap(Reference first, Reference second) {
            const T value = first;
            first = double(T(second));
            second = double(value);
        }
      private:
        Reference(std::shared_ptr<View> view, std::size_t index) : view_(std::move(view)), index_(index) {}
        std::shared_ptr<View> view_;
        std::size_t index_;
        friend class TypedArray;
    };
    Reference operator[](std::size_t index) { return Reference(view_, index); }
    T operator[](std::size_t index) const { return load(index); }
    T at(std::size_t index) const {
        if (index >= size()) throw std::out_of_range("TypedArray::at");
        return load(index);
    }
    JSValue get(double index) const {
        if (!validIndex(index)) return JSValue();
        return JSValue(load(std::size_t(index)));
    }
    void setIndex(double index, double value) {
        if (validIndex(index)) store(std::size_t(index), value);
    }
    TypedArray subarray(double begin = 0, std::optional<double> end = std::nullopt) const {
        const auto first = ArrayBuffer::relativeIndex(begin, size());
        const auto last = end ? ArrayBuffer::relativeIndex(*end, size()) : size();
        return TypedArray(buffer, double(std::size_t(byteOffset) + first * sizeof(T)),
                          double(last > first ? last - first : 0));
    }
    TypedArray slice(double begin = 0, std::optional<double> end = std::nullopt) const {
        const auto source = subarray(begin, end);
        TypedArray result(source.size());
        if (!source.empty()) std::memcpy(result.buffer.bytes_->data(),
            source.buffer.bytes_->data() + std::size_t(source.byteOffset),
            std::size_t(source.byteLength));
        return result;
    }
    template <class U, bool C> void set(const TypedArray<U, C>& source, double offset = 0) {
        const auto start = checkedOffset(offset, source.size());
        if constexpr (std::is_same_v<T, U> && Clamped == C) {
            if (!source.empty()) std::memmove(buffer.bytes_->data() + std::size_t(byteOffset) + start * sizeof(T),
                source.buffer.bytes_->data() + std::size_t(source.byteOffset), source.size() * sizeof(T));
            return;
        }
        // Snapshot first, including overlapping differently typed views.
        std::vector<double> values;
        values.reserve(source.size());
        for (const auto value : source) values.push_back(double(value));
        for (std::size_t i = 0; i < values.size(); ++i) store(start + i, values[i]);
    }
    template <class U> void set(const std::vector<U>& source, double offset = 0) {
        const auto start = checkedOffset(offset, source.size());
        for (std::size_t i = 0; i < source.size(); ++i) store(start + i, double(source[i]));
    }
    void assign(std::size_t count, double value) {
        if (count != size()) throw JSRangeError("TypedArray storage has fixed length");
        for (std::size_t i = 0; i < count; ++i) store(i, value);
    }
    template <bool Const> class Iterator {
      public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::random_access_iterator_tag;
        using pointer = void;
        using reference = std::conditional_t<Const, T, Reference>;
        Iterator() = default;
        Iterator(std::shared_ptr<View> view, difference_type index) : view_(std::move(view)), index_(index) {}
        template <bool Other> requires (Const && !Other)
        Iterator(const Iterator<Other>& other) : view_(other.view_), index_(other.index_) {}
        reference operator*() const {
            if constexpr (Const) return loadView(*view_, std::size_t(index_));
            else return Reference(view_, std::size_t(index_));
        }
        reference operator[](difference_type offset) const { return *(*this + offset); }
        Iterator& operator++() { ++index_; return *this; }
        Iterator operator++(int) { auto old = *this; ++*this; return old; }
        Iterator& operator--() { --index_; return *this; }
        Iterator operator--(int) { auto old = *this; --*this; return old; }
        Iterator& operator+=(difference_type amount) { index_ += amount; return *this; }
        Iterator& operator-=(difference_type amount) { index_ -= amount; return *this; }
        Iterator operator+(difference_type amount) const { auto result = *this; return result += amount; }
        Iterator operator-(difference_type amount) const { auto result = *this; return result -= amount; }
        friend Iterator operator+(difference_type amount, Iterator value) { return value += amount; }
        difference_type operator-(const Iterator& other) const { return index_ - other.index_; }
        bool operator==(const Iterator& other) const { return view_ == other.view_ && index_ == other.index_; }
        bool operator<(const Iterator& other) const { return index_ < other.index_; }
        bool operator>(const Iterator& other) const { return other < *this; }
        bool operator<=(const Iterator& other) const { return !(other < *this); }
        bool operator>=(const Iterator& other) const { return !(*this < other); }
      private:
        std::shared_ptr<View> view_;
        difference_type index_ = 0;
        template <bool> friend class Iterator;
    };
    Iterator<false> begin() { return {view_, 0}; }
    Iterator<false> end() { return {view_, std::ptrdiff_t(size())}; }
    Iterator<true> begin() const { return {view_, 0}; }
    Iterator<true> end() const { return {view_, std::ptrdiff_t(size())}; }
  private:
    std::shared_ptr<View> view_;
    explicit TypedArray(std::shared_ptr<View> view)
        : buffer_(view->buffer), buffer(buffer_), byteOffset(view->offset),
          byteLength(view->count * sizeof(T)), length(view->count), view_(std::move(view)) {}
    friend struct NativeArrayAccess;
    static std::size_t checkedBytes(std::size_t count) {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw JSRangeError("TypedArray length overflows storage");
        return count * sizeof(T);
    }
    bool validIndex(double index) const {
        return std::isfinite(index) && index >= 0 && std::trunc(index) == index && index < double(size());
    }
    std::size_t checkedOffset(double offset, std::size_t count) const {
        if (std::isnan(offset)) offset = 0;
        offset = std::trunc(offset);
        if (offset < 0 || offset > double(size()) || !std::isfinite(offset))
            throw JSRangeError("TypedArray.set offset is out of bounds");
        const auto start = std::size_t(offset);
        if (count > size() - start) throw JSRangeError("TypedArray.set source is too large");
        return start;
    }
    T load(std::size_t index) const { return loadView(*view_, index); }
    void store(std::size_t index, double value) { storeView(*view_, index, value); }
    static T loadView(const View& view, std::size_t index) {
        T value{};
        if (index < view.count) std::memcpy(&value, view.buffer.bytes_->data() + view.offset + index * sizeof(T), sizeof(T));
        return value;
    }
    static void storeView(View& view, std::size_t index, double value) {
        if (index >= view.count) return;
        const T result = convert(value);
        std::memcpy(view.buffer.bytes_->data() + view.offset + index * sizeof(T), &result, sizeof(T));
    }
    static T convert(double value) {
        if constexpr (Clamped) {
            if (std::isnan(value) || value <= 0) return 0;
            if (value >= 255) return 255;
            const double low = std::floor(value), fraction = value - low;
            return T(low + (fraction > .5 || (fraction == .5 && std::fmod(low, 2) != 0) ? 1 : 0));
        } else if constexpr (std::is_same_v<T, float>) {
            // Avoid out-of-range C++ conversion while preserving IEEE round-to-nearest overflow.
            constexpr double overflow = 0x1.ffffffp127;
            constexpr double largest = double(std::numeric_limits<float>::max());
            if (value > largest) return value >= overflow ? std::numeric_limits<float>::infinity()
                                                         : std::numeric_limits<float>::max();
            if (value < -largest) return value <= -overflow ? -std::numeric_limits<float>::infinity()
                                                           : -std::numeric_limits<float>::max();
            return static_cast<float>(value);
        } else if constexpr (std::is_same_v<T, double>) return value;
        else {
            if (!std::isfinite(value) || value == 0) return 0;
            constexpr double modulus = double(std::uint64_t{1} << (sizeof(T) * 8));
            double wrapped = std::fmod(std::trunc(value), modulus);
            if (wrapped < 0) wrapped += modulus;
            if constexpr (std::is_signed_v<T>) if (wrapped >= modulus / 2) wrapped -= modulus;
            return static_cast<T>(wrapped);
        }
    }
};

using Int8Array = TypedArray<std::int8_t>;
using Uint8Array = TypedArray<std::uint8_t>;
using Uint8ClampedArray = TypedArray<std::uint8_t, true>;
using Int16Array = TypedArray<std::int16_t>;
using Uint16Array = TypedArray<std::uint16_t>;
using Int32Array = TypedArray<std::int32_t>;
using Uint32Array = TypedArray<std::uint32_t>;
using Float32Array = TypedArray<float>;
using Float64Array = TypedArray<double>;

#pragma once

// Native port of examples/jsm/utils/SortUtils.js from the pinned three.js revision
// (see upstream.json). The module exports one hybrid radix sort that expects unsigned
// 32-bit integer keys and is used by addons that need depth ordering of thousands of
// entries (BatchedMesh instancing, point and splat rendering).
//
// The JavaScript module keeps its scratch bins in module scope:
//
//     const bins = new Array( ITERATIONS );
//     const bins_buffer = new ArrayBuffer( ( ITERATIONS + 1 ) * BIN_SIZE * 4 );
//
// so every call shares the same counting bins. The native port keeps that state as one
// thread-local storage block, following the module-temporary rule in docs/PORTING.md.
//
// Elements are native values; the optional `get` callback mirrors `options.get` and the
// `>>> 0` conversion applied to every key is part of the algorithm, not the caller.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace SortUtils {

constexpr int POWER = 3;
constexpr int BIT_MAX = 32;
constexpr int BIN_BITS = 1 << POWER;
constexpr int BIN_SIZE = 1 << BIN_BITS;
constexpr int BIN_MAX = BIN_SIZE - 1;
constexpr int ITERATIONS = BIT_MAX / BIN_BITS;

// One ArrayBuffer holding ITERATIONS + 1 Uint32Array( BIN_SIZE ) views.
struct BinStorage {
    std::array<std::uint32_t, std::size_t(ITERATIONS + 1) * BIN_SIZE> values{};

    std::uint32_t* bin(int index);
};

// The shared module-scope bins, one storage block per owning thread.
BinStorage& moduleBins();

// ToUint32 applied to every key read through `get`, matching `get( value ) >>> 0`.
std::uint32_t toUint32(double value);

// options.get defaults to the identity arrow function `( el ) => el`.
struct IdentityGet {
    template <class T> double operator()(const T& value) const { return double(value); }
};

template <class T> struct RadixSortOptions {
    // options.aux: caller-owned scratch storage. A null pointer allocates the private
    // aux array that `new arr.constructor( len )` creates upstream.
    std::vector<T>* aux = nullptr;
    // options.reversed: descending key order.
    bool reversed = false;
};

namespace detail {
template <class T, class Get> class RadixSortRun {
    Get& get;
    const bool reversed;
    // options.aux storage feeds the alternating `const data = [ arr, aux ]` pair.
    std::vector<T>* data[2];

    std::uint32_t key(const T& element) const { return toUint32(double(get(element))); }
    // compare( a, b ) is `a > b` ascending and `a < b` reversed.
    bool compare(std::uint32_t left, std::uint32_t right) const {
        return reversed ? left < right : left > right;
    }
    void accumulate(std::uint32_t* bin) const {
        if (reversed) {
            for (int j = BIN_SIZE - 2; j >= 0; --j) bin[j] += bin[j + 1];
        } else {
            for (int j = 1; j < BIN_SIZE; ++j) bin[j] += bin[j - 1];
        }
    }
    void insertionSortBlock(int depth, std::ptrdiff_t start, std::ptrdiff_t length) {
        std::vector<T>& a = *data[depth & 1];
        std::vector<T>& b = *data[(depth + 1) & 1];
        for (std::ptrdiff_t j = start + 1; j < start + length; ++j) {
            const T p = a[std::size_t(j)];
            const std::uint32_t t = key(p);
            std::ptrdiff_t i = j;
            while (i > start) {
                // `a[ i ] = a[ -- i ]` evaluates the target index before the
                // decrement in JavaScript; C++ sequences the right operand first,
                // so the shift is written out explicitly.
                if (compare(key(a[std::size_t(i - 1)]), t)) {
                    a[std::size_t(i)] = a[std::size_t(i - 1)];
                    --i;
                } else {
                    break;
                }
            }
            a[std::size_t(i)] = p;
        }
        if ((depth & 1) == 1) {
            for (std::ptrdiff_t i = start; i < start + length; ++i) b[std::size_t(i)] = a[std::size_t(i)];
        }
    }
    void recurse(const std::uint32_t* cache, int depth, std::ptrdiff_t start) {
        std::ptrdiff_t prev = 0;
        const auto visit = [&](int j) {
            const std::ptrdiff_t cur = std::ptrdiff_t(cache[j]), diff = cur - prev;
            if (diff != 0) {
                if (diff > 32)
                    radixSortBlock(depth + 1, start + prev, diff);
                else
                    insertionSortBlock(depth + 1, start + prev, diff);
                prev = cur;
            }
        };
        if (reversed) {
            for (int j = BIN_MAX; j >= 0; --j) visit(j);
        } else {
            for (int j = 0; j < BIN_SIZE; ++j) visit(j);
        }
    }
    void radixSortBlock(int depth, std::ptrdiff_t start, std::ptrdiff_t length) {
        std::vector<T>& a = *data[depth & 1];
        std::vector<T>& b = *data[(depth + 1) & 1];
        const int shift = (3 - depth) << POWER;
        const std::ptrdiff_t end = start + length;
        std::uint32_t* const cache = moduleBins().bin(depth);
        std::uint32_t* const bin = moduleBins().bin(depth + 1);
        std::fill(bin, bin + BIN_SIZE, 0u);
        for (std::ptrdiff_t j = start; j < end; ++j)
            bin[(key(a[std::size_t(j)]) >> shift) & BIN_MAX]++;
        accumulate(bin);
        std::copy(bin, bin + BIN_SIZE, cache);
        for (std::ptrdiff_t j = end - 1; j >= start; --j) {
            // `b[ start + -- bin[ ... ] ] = a[ j ]`: the pre-decrement selects the
            // target slot before the element is read.
            std::uint32_t& cursor = bin[(key(a[std::size_t(j)]) >> shift) & BIN_MAX];
            --cursor;
            b[std::size_t(start + std::ptrdiff_t(cursor))] = a[std::size_t(j)];
        }
        if (depth == ITERATIONS - 1) return;
        recurse(cache, depth, start);
    }

  public:
    RadixSortRun(std::vector<T>& array, std::vector<T>& aux, bool reversed, Get& get)
        : get(get), reversed(reversed), data{&array, &aux} {}
    void sort() { radixSortBlock(0, 0, std::ptrdiff_t(data[0]->size())); }
};
} // namespace detail

// radixSort( arr, opt ). The sorted result is left in `array` by the closing pass, the
// same way the upstream implementation leaves it in the caller's array; `options.aux`
// receives the intermediate passes exactly like the JavaScript scratch argument.
template <class T, class Get = IdentityGet>
void radixSort(std::vector<T>& array, const RadixSortOptions<T>& options = {}, Get get = {}) {
    std::vector<T> owned;
    std::vector<T>* aux = options.aux;
    if (aux == nullptr) {
        owned.resize(array.size());
        aux = &owned;
    }
    detail::RadixSortRun<T, Get>(array, *aux, options.reversed, get).sort();
}

} // namespace SortUtils

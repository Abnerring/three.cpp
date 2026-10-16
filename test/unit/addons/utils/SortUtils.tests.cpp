#include "catch.hpp"
#include "utils/SortUtils.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
std::uint32_t key(double value) { return SortUtils::toUint32(value); }
bool ascending(const std::vector<double>& values) {
    for (std::size_t i = 1; i < values.size(); ++i)
        if (key(values[i - 1]) > key(values[i])) return false;
    return true;
}
bool descending(const std::vector<double>& values) {
    for (std::size_t i = 1; i < values.size(); ++i)
        if (key(values[i - 1]) < key(values[i])) return false;
    return true;
}
struct Item {
    double key;
    int id;
};
}

TEST_CASE("SortUtils radixSort orders unsigned 32 bit keys ascending", "[sortutils]") {
    std::vector<double> values{3, 1, 2};
    SortUtils::radixSort(values);
    REQUIRE(values == std::vector<double>{1, 2, 3});
    std::vector<double> random{4294967295., 0., 65536., 255., 4294967294., 256.};
    SortUtils::radixSort(random);
    REQUIRE(ascending(random));
    REQUIRE(random.front() == 0.);
    REQUIRE(random.back() == 4294967295.);
}

TEST_CASE("SortUtils radixSort honours the reversed flag", "[sortutils]") {
    std::vector<double> values;
    for (int i = 0; i < 300; ++i) values.push_back(double((i * 7919) % 1024));
    std::vector<double> forward = values, backward = values;
    SortUtils::radixSort(forward);
    SortUtils::radixSort(backward, SortUtils::RadixSortOptions<double>{nullptr, true});
    REQUIRE(ascending(forward));
    REQUIRE(descending(backward));
    REQUIRE(forward.front() == backward.back());
    REQUIRE(forward.back() == backward.front());
}

TEST_CASE("SortUtils radixSort applies the >>> 0 key conversion", "[sortutils]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> single{nan};
    SortUtils::radixSort(single);
    REQUIRE(single.size() == 1);
    REQUIRE(std::isnan(single[0]));
    for (const double value : {inf, -inf, -0., -1., -4294967296., 4294967296. * 3 + 5, 7.9, -7.9}) {
        std::vector<double> one{value};
        SortUtils::radixSort(one);
        REQUIRE(one.size() == 1);
        REQUIRE(one[0] == value);
        const double truncated = std::isfinite(value) ? std::trunc(value) : 0.;
        const double wrapped = std::fmod(std::fmod(truncated, 4294967296.) + 4294967296., 4294967296.);
        REQUIRE(SortUtils::toUint32(value) == std::uint32_t(wrapped));
    }
    // Fractions truncate, negatives wrap and non-finite values behave as zero.
    std::vector<double> values{nan, 2.9, -1., inf, 0., 4294967296. + 4.};
    SortUtils::radixSort(values);
    for (std::size_t i = 1; i < values.size(); ++i) REQUIRE(key(values[i - 1]) <= key(values[i]));
    // Counting and insertion passes are stable, so the three zero keys keep their input order.
    REQUIRE(key(values[0]) == 0u);
    REQUIRE(key(values[1]) == 0u);
    REQUIRE(key(values[2]) == 0u);
    REQUIRE(key(values[3]) == 2u);
    REQUIRE(key(values[4]) == 4u);
    REQUIRE(key(values[5]) == 4294967295u);
    REQUIRE(std::isnan(values[0]));
    REQUIRE(values[2] == 0.);
    REQUIRE(values[3] == 2.9);
    REQUIRE(values[5] == -1.);
}

TEST_CASE("SortUtils radixSort reads keys through a getter", "[sortutils]") {
    std::vector<Item> items{{5, 0}, {1, 1}, {3, 2}, {5, 3}, {0, 4}};
    SortUtils::radixSort(items, SortUtils::RadixSortOptions<Item>{}, [](const Item& item) { return item.key; });
    REQUIRE(items.size() == 5);
    for (std::size_t i = 1; i < items.size(); ++i) REQUIRE(items[i - 1].key <= items[i].key);
    // Equal keys keep their input order.
    REQUIRE(items[3].key == 5);
    REQUIRE(items[4].key == 5);
    REQUIRE(items[3].id == 0);
    REQUIRE(items[4].id == 3);
    std::vector<Item> scaled{{2, 0}, {1, 1}};
    SortUtils::radixSort(scaled, SortUtils::RadixSortOptions<Item>{},
        [](const Item& item) { return item.key * 1000000000.; });
    REQUIRE(scaled[0].id == 1);
    REQUIRE(scaled[1].id == 0);
}

TEST_CASE("SortUtils radixSort uses caller scratch and keeps the result in the array", "[sortutils]") {
    std::vector<double> values{9, 4, 7, 1, 8, 2};
    std::vector<double> aux{1, 2, 3, 4, 5, 6};
    SortUtils::RadixSortOptions<double> options;
    options.aux = &aux;
    SortUtils::radixSort(values, options);
    REQUIRE(ascending(values));
    REQUIRE(values == std::vector<double>{1, 2, 4, 7, 8, 9});
    REQUIRE(aux.size() == 6);
    REQUIRE(aux != std::vector<double>{1, 2, 3, 4, 5, 6});
}

TEST_CASE("SortUtils radixSort handles empty and single element input", "[sortutils]") {
    std::vector<double> empty;
    SortUtils::radixSort(empty);
    REQUIRE(empty.empty());
    std::vector<double> single{42.};
    SortUtils::radixSort(single);
    REQUIRE(single == std::vector<double>{42.});
}

TEST_CASE("SortUtils radixSort leaves the array sorted for the pinned boundaries", "[sortutils]") {
    for (const std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(31), std::size_t(32),
             std::size_t(33), std::size_t(255), std::size_t(256), std::size_t(257), std::size_t(1000)}) {
        std::vector<double> values;
        std::uint32_t state = 0x12345678u;
        for (std::size_t i = 0; i < count; ++i) {
            state = state * 1664525u + 1013904223u;
            values.push_back(double(state));
        }
        std::vector<double> reference = values;
        SortUtils::radixSort(values);
        std::stable_sort(reference.begin(), reference.end(),
            [](double a, double b) { return key(a) < key(b); });
        REQUIRE(values.size() == reference.size());
        for (std::size_t i = 0; i < values.size(); ++i) REQUIRE(key(values[i]) == key(reference[i]));
    }
}

TEST_CASE("SortUtils module bins are shared and reset between calls", "[sortutils]") {
    std::vector<double> first{5, 3, 1}, second;
    for (int i = 0; i < 512; ++i) second.push_back(double(512 - i));
    SortUtils::radixSort(first);
    SortUtils::radixSort(second);
    REQUIRE(first == std::vector<double>{1, 3, 5});
    REQUIRE(ascending(second));
    SortUtils::radixSort(first, SortUtils::RadixSortOptions<double>{nullptr, true});
    REQUIRE(first == std::vector<double>{5, 3, 1});
}

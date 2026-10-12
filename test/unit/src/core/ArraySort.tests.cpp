#include "catch.hpp"
#include "core/ArraySort.h"
#include <memory>

TEST_CASE("Pinned V8 sorting preserves short-array comparison order", "[arraysort]") {
    std::vector<int> values{4, 1, 3, 2};
    std::vector<std::pair<int, int>> comparisons;
    v8ArraySort(values, [&](int a, int b) {
        comparisons.emplace_back(a, b);
        return double(a) - b;
    });
    REQUIRE(values == std::vector<int>{1, 2, 3, 4});
    REQUIRE(comparisons == std::vector<std::pair<int, int>>{{1,4},{3,4},{3,1},{2,3},{2,1}});
}

TEST_CASE("Pinned V8 sorting handles a nontransitive NaN comparison sequence", "[arraysort]") {
    const std::vector<double> keys{7, NAN, 3, 1, NAN, 9, 0, 8, 4, NAN, 2, 6, 5};
    std::vector<int> values;
    for (int i = 0; i < int(keys.size()); ++i) values.push_back(i);
    v8ArraySort(values, [&](int a, int b) { return keys[a] - keys[b]; });
    REQUIRE(values == std::vector<int>{0,1,6,3,2,4,10,8,12,11,7,5,9});
}

TEST_CASE("Pinned V8 sorting preserves shared values and comparator-appended elements", "[arraysort]") {
    struct Item { int key, id; };
    std::vector<std::shared_ptr<Item>> values;
    for (int i = 0; i < 128; ++i) values.push_back(std::make_shared<Item>(Item{i % 9, i}));
    auto appended = std::make_shared<Item>(Item{-1, 128});
    bool first = true;
    v8ArraySort(values, [&](const auto& a, const auto& b) {
        if (first) { first = false; values.push_back(appended); }
        return double(a->key) - b->key;
    });
    REQUIRE(values.size() == 129);
    REQUIRE(values.back() == appended);
    for (std::size_t i = 1; i < 128; ++i) {
        REQUIRE(values[i - 1]->key <= values[i]->key);
        if (values[i - 1]->key == values[i]->key) REQUIRE(values[i - 1]->id < values[i]->id);
    }
}

TEST_CASE("Pinned V8 sorting leaves receiver order intact when a comparator throws", "[arraysort]") {
    std::vector<int> values{4, 1, 3, 2};
    int calls = 0;
    REQUIRE_THROWS_AS(v8ArraySort(values, [&](int a, int b) {
        if (++calls == 3) throw std::runtime_error("comparator");
        return double(a) - b;
    }), std::runtime_error);
    REQUIRE(values == std::vector<int>{4, 1, 3, 2});
}

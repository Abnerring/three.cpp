#include "nodes/core/NodeUtils.h"

#include "catch.hpp"

#include <vector>

// The expected hash values were produced by running the pinned NodeUtils.js in node, so
// these are bit exact comparisons against the upstream implementation.
TEST_CASE("hashString matches the upstream cyrb53 bit for bit") {
    REQUIRE(hashString("abc") == 5059922895146125.0);
    REQUIRE(hashString("") == 3338908027751811.0);
    // Order matters: a permutation of the same characters is a different hash.
    REQUIRE(hashString("acb") != hashString("abc"));
}

TEST_CASE("hashArray and hash agree with upstream and with each other") {
    const std::vector<double> values{1, 2, 3};
    REQUIRE(hashArray(values) == 4235446385759781.0);
    REQUIRE(hash(values) == 4235446385759781.0);
}

TEST_CASE("hashArray applies JavaScript number to int32 conversion") {
    // Negatives, fractions and values past the int32 range all wrap the way ToInt32 does.
    const std::vector<double> values{-5, 0.5, 1e9};
    REQUIRE(hashArray(values) == 2529312072353790.0);
}

TEST_CASE("nodeImul wraps like Math.imul") {
    REQUIRE(nodeImul(0x7fffffff, 2) == -2);
    REQUIRE(nodeImul(-1, 1) == -1);
    REQUIRE(nodeImul(65536, 65536) == 0);
}

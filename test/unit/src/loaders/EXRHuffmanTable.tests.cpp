#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("EXRLoader leaves a missing symbol alone") {
    // A length of zero means the symbol does not appear and is not given a code.
    std::vector<int> lengths{0, 0, 0};
    EXRLoader::hufCanonicalCodeTable(lengths);
    REQUIRE(lengths[0] == 0);
    REQUIRE(lengths[2] == 0);
}

TEST_CASE("EXRLoader gives two equal lengths consecutive codes") {
    // Two codes of length one are numbered zero and one.
    std::vector<int> lengths{1, 1};
    EXRLoader::hufCanonicalCodeTable(lengths);
    REQUIRE(EXRLoader::hufLength(lengths[0]) == 1);
    REQUIRE(EXRLoader::hufCode(lengths[0]) == 0);
    REQUIRE(EXRLoader::hufLength(lengths[1]) == 1);
    REQUIRE(EXRLoader::hufCode(lengths[1]) == 1);
    // Their packed forms are the length with the code above it.
    REQUIRE(lengths[0] == 1);
    REQUIRE(lengths[1] == 65);
}

TEST_CASE("EXRLoader starts a longer length at zero") {
    // Two codes of length two take the numbers zero and one, the same as the length one pair
    // would, because the walk down from the longest reaches them first.
    std::vector<int> lengths{2, 2};
    EXRLoader::hufCanonicalCodeTable(lengths);
    REQUIRE(EXRLoader::hufLength(lengths[0]) == 2);
    REQUIRE(EXRLoader::hufCode(lengths[0]) == 0);
    REQUIRE(EXRLoader::hufCode(lengths[1]) == 1);
}

TEST_CASE("EXRLoader lays out a mixed set as canonical codes") {
    // One code of length one and two of length two: the short one takes the number one and the
    // two long ones take zero and one, which is the only way the three can share the tree.
    std::vector<int> lengths{1, 2, 2};
    EXRLoader::hufCanonicalCodeTable(lengths);
    REQUIRE(EXRLoader::hufLength(lengths[0]) == 1);
    REQUIRE(EXRLoader::hufCode(lengths[0]) == 1);
    REQUIRE(EXRLoader::hufLength(lengths[1]) == 2);
    REQUIRE(EXRLoader::hufCode(lengths[1]) == 0);
    REQUIRE(EXRLoader::hufCode(lengths[2]) == 1);
    // The order of the two long ones follows the order they were given in.
    REQUIRE(EXRLoader::hufCode(lengths[2]) > EXRLoader::hufCode(lengths[1]));
}

TEST_CASE("EXRLoader keeps every assigned code within its length") {
    // Whatever lengths go in, every code that comes out fits inside the length it claims.
    std::vector<int> lengths{3, 3, 2, 4, 4, 4, 4, 1};
    EXRLoader::hufCanonicalCodeTable(lengths);
    for (const int entry : lengths) {
        REQUIRE(EXRLoader::hufLength(entry) > 0);
        REQUIRE(EXRLoader::hufEntryValid(entry));
    }
}
#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <array>
#include <set>

TEST_CASE("EXRLoader holds sixty four zigzag positions") {
    // One per number in an eight by eight block.
    REQUIRE(EXRLoader::unZigZagOrder().size() == 64);
    // The first two and the last two are the identity.
    REQUIRE(EXRLoader::unZigZagOrder()[0] == 0);
    REQUIRE(EXRLoader::unZigZagOrder()[1] == 1);
    REQUIRE(EXRLoader::unZigZagOrder()[62] == 62);
    REQUIRE(EXRLoader::unZigZagOrder()[63] == 63);
}

TEST_CASE("EXRLoader walks the block in diagonals") {
    // The first few positions step out along the top row and back.
    REQUIRE(EXRLoader::unZigZagIndex(0) == 0);
    REQUIRE(EXRLoader::unZigZagIndex(1) == 1);
    REQUIRE(EXRLoader::unZigZagIndex(2) == 5);
    REQUIRE(EXRLoader::unZigZagIndex(3) == 6);
    REQUIRE(EXRLoader::unZigZagIndex(4) == 14);
    // Then the next diagonal starts back near the top.
    REQUIRE(EXRLoader::unZigZagIndex(8) == 2);
    // The sequence is therefore not monotonic.
    REQUIRE(EXRLoader::unZigZagIndex(8) < EXRLoader::unZigZagIndex(7));
}

TEST_CASE("EXRLoader holds a permutation") {
    // Every position from zero to sixty three appears exactly once.
    std::set<int> seen;
    for (const int value : EXRLoader::unZigZagOrder()) {
        REQUIRE(value >= 0);
        REQUIRE(value < 64);
        seen.insert(value);
    }
    REQUIRE(seen.size() == 64);
}

TEST_CASE("EXRLoader refuses an index outside the block") {
    // Below zero and past the end read nothing.
    REQUIRE(EXRLoader::unZigZagIndex(-1) == -1);
    REQUIRE(EXRLoader::unZigZagIndex(64) == -1);
    REQUIRE(EXRLoader::unZigZagIndex(1000) == -1);
    // The two ends themselves are inside it.
    REQUIRE(EXRLoader::unZigZagIndex(0) == 0);
    REQUIRE(EXRLoader::unZigZagIndex(63) == 63);
}
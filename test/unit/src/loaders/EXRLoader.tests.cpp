#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader splits a huffman entry") {
    // The low six bits are the length.
    REQUIRE(EXRLoader::hufLength(0) == 0);
    REQUIRE(EXRLoader::hufLength(63) == 63);
    // The seventh bit already belongs to the code, not the length.
    REQUIRE(EXRLoader::hufLength(64) == 0);
    REQUIRE(EXRLoader::hufLength(127) == 63);
    // The rest is the code.
    REQUIRE(EXRLoader::hufCode(0) == 0);
    REQUIRE(EXRLoader::hufCode(64) == 1);
    REQUIRE(EXRLoader::hufCode(127) == 1);
}

TEST_CASE("EXRLoader packs and splits the same way") {
    // A length of five and a code of sixty three round trip.
    const int packed_ = (63 << 6) | 5;
    REQUIRE(EXRLoader::hufLength(packed_) == 5);
    REQUIRE(EXRLoader::hufCode(packed_) == 63);
    // The length does not disturb the code.
    REQUIRE(EXRLoader::hufCode((7 << 6) | 0) == EXRLoader::hufCode((7 << 6) | 63));
}

TEST_CASE("EXRLoader refuses an unusable entry") {
    // A length of zero cannot hold a code.
    REQUIRE_FALSE(EXRLoader::hufEntryValid(0));
    // A length of sixty three with a code of zero is valid: the shift is long but the
    // code needs none of it.
    REQUIRE(EXRLoader::hufEntryValid(63));
    // One bit of length holds the code one.
    REQUIRE(EXRLoader::hufEntryValid((1 << 6) | 1));
    REQUIRE(EXRLoader::hufEntryValid((0 << 6) | 1));
    // The same one bit cannot hold the code three, which needs two.
    REQUIRE_FALSE(EXRLoader::hufEntryValid((3 << 6) | 1));
    // Two bits hold it.
    REQUIRE(EXRLoader::hufEntryValid((3 << 6) | 2));
}

TEST_CASE("EXRLoader knows the file magic and version") {
    // The magic and the version the format begins with.
    REQUIRE(EXRLoader::kMagic == 20000630);
    REQUIRE(EXRLoader::kVersion == 2);
}

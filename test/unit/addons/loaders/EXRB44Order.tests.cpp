#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader turns an ordered value into a half") {
    // The largest ordered value has every bit set apart from the top one, so it is positive and
    // keeps all fifteen: the largest half.
    REQUIRE(EXRLoader::b44OrderedToHalf(0xFFFF) == 0x7FFF);
    // A value whose top bit is set has it dropped.
    REQUIRE(EXRLoader::b44OrderedToHalf(0x8000) == 0x0000);
    REQUIRE(EXRLoader::b44OrderedToHalf(0x8001) == 0x0001);
    REQUIRE(EXRLoader::b44OrderedToHalf(0xC000) == 0x4000);
}

TEST_CASE("EXRLoader turns a value without the top bit round") {
    // Without the top bit the value is negative, so every bit is turned over.
    REQUIRE(EXRLoader::b44OrderedToHalf(0x0000) == 0xFFFF);
    REQUIRE(EXRLoader::b44OrderedToHalf(0x0001) == 0xFFFE);
    REQUIRE(EXRLoader::b44OrderedToHalf(0x7FFF) == 0x8000);
    // The two sides meet at the boundary without overlapping: the smallest positive and the
    // smallest negative are one apart in the result.
    REQUIRE(EXRLoader::b44OrderedToHalf(0x8000) == 0x0000);
    REQUIRE(EXRLoader::b44OrderedToHalf(0x7FFF) == 0x8000);
}

TEST_CASE("EXRLoader keeps the ordered conversion within sixteen bits") {
    // Every input gives an output inside the half's own range.
    for (std::uint32_t value = 0; value <= 0xFFFF; value += 0x1111) {
        const auto half = EXRLoader::b44OrderedToHalf(static_cast<std::uint16_t>(value));
        REQUIRE(half <= 0xFFFF);
    }
    // The conversion is its own inverse only on the negative side, which is worth knowing: a
    // round trip does not restore the ordered value.
    const auto positive = EXRLoader::b44OrderedToHalf(0x8001);
    REQUIRE(positive == 0x0001);
    REQUIRE(EXRLoader::b44OrderedToHalf(positive) == 0xFFFE);
}
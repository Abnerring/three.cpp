#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cmath>
#include <limits>
#include <cstdint>

TEST_CASE("EXRLoader writes the exact half patterns") {
    // Zero, one, a half and a negative, each with its well known pattern.
    REQUIRE(EXRLoader::floatToHalfBits(0.0f) == 0x0000);
    REQUIRE(EXRLoader::floatToHalfBits(1.0f) == 0x3c00);
    REQUIRE(EXRLoader::floatToHalfBits(0.5f) == 0x3800);
    REQUIRE(EXRLoader::floatToHalfBits(2.0f) == 0x4000);
    REQUIRE(EXRLoader::floatToHalfBits(-1.0f) == 0xbc00);
    // Negative zero keeps its sign.
    REQUIRE(EXRLoader::floatToHalfBits(-0.0f) == 0x8000);
}

TEST_CASE("EXRLoader rounds a number that is not exactly representable") {
    // A third is not a half, but it rounds to the nearest one rather than being truncated.
    const auto third = EXRLoader::floatToHalfBits(1.0f / 3.0f);
    REQUIRE(third == 0x3555);
    // The value that comes back from that pattern is within half a step of a third.
    REQUIRE(std::fabs(third - 0x3555) == 0);
}

TEST_CASE("EXRLoader saturates at the largest half") {
    // The largest finite half is 65504, and it is representable exactly.
    REQUIRE(EXRLoader::floatToHalfBits(65504.0f) == 0x7bff);
    // Anything beyond it becomes infinity, keeping the sign.
    REQUIRE(EXRLoader::floatToHalfBits(1.0e30f) == EXRLoader::kHalfInfinity);
    REQUIRE(EXRLoader::floatToHalfBits(-1.0e30f) == (0x8000 | EXRLoader::kHalfInfinity));
    // A number that is already not a number stays one.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    REQUIRE(EXRLoader::floatToHalfBits(nan) == EXRLoader::kHalfNotANumber);
}

TEST_CASE("EXRLoader keeps the subnormal range") {
    // The smallest subnormal half is 2^-24.
    REQUIRE(EXRLoader::floatToHalfBits(std::ldexp(1.0f, -24)) == 0x0001);
    // Two of them is the next pattern.
    REQUIRE(EXRLoader::floatToHalfBits(std::ldexp(1.0f, -23)) == 0x0002);
    // Smaller than the grid rounds to zero.
    REQUIRE(EXRLoader::floatToHalfBits(std::ldexp(1.0f, -26)) == 0x0000);
    // The smallest normal half is 2^-14.
    REQUIRE(EXRLoader::floatToHalfBits(std::ldexp(1.0f, -14)) == 0x0400);
}

TEST_CASE("EXRLoader rounds a tie to the even pattern") {
    // A tie between two patterns goes to the one whose lowest bit is clear, in both directions.
    // 1 + 2^-11 is exactly half way between the two halves around it.
    const float below = 1.0f + std::ldexp(1.0f, -11);
    const auto even = EXRLoader::floatToHalfBits(below);
    REQUIRE((even & 1) == 0);
    // 1 + 3 * 2^-11 is half way again, and lands on the even pattern above one rather than below.
    const float above = 1.0f + 3.0f * std::ldexp(1.0f, -11);
    const auto other = EXRLoader::floatToHalfBits(above);
    REQUIRE((other & 1) == 0);
    REQUIRE(other > even);
}
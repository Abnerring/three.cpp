#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader keeps the narrow wavelet inside sixteen bits") {
    // The two constants the narrow form works with.
    REQUIRE(EXRLoader::kWaveletModMask == 0xffff);
    REQUIRE(EXRLoader::kWaveletOffset == 32768);
    // Zero and zero: the half of zero is zero, so the second is zero and the first is the
    const auto zero = EXRLoader::wdec16(0, 0);
    REQUIRE(zero.b == 0);
    // The offset taken away from zero wraps back round to the offset itself.
    REQUIRE(zero.a == 32768);
}

TEST_CASE("EXRLoader wraps rather than clamping in the narrow form") {
    // One and zero: the difference halved is zero, so the second is the first, and the first
    // goes below zero by the offset and comes back round.
    const auto one = EXRLoader::wdec16(1, 0);
    REQUIRE(one.b == 1);
    REQUIRE(one.a == static_cast<int>((0 + 1 - 32768) & 0xffff));
    REQUIRE(one.a == 32769);
    // The result never leaves the sixteen bit range, however far the arithmetic would go.
    const auto extreme = EXRLoader::wdec16(0xffff, 0xffff);
    REQUIRE(extreme.a >= 0);
    REQUIRE(extreme.a <= 0xffff);
    REQUIRE(extreme.b >= 0);
    REQUIRE(extreme.b <= 0xffff);
}

TEST_CASE("EXRLoader tests the leftover row by bit too") {
    // The same bit test as the column one, on the other side.
    REQUIRE_FALSE(EXRLoader::pizHasOddRow(2, 1));
    REQUIRE(EXRLoader::pizHasOddRow(3, 1));
    REQUIRE_FALSE(EXRLoader::pizHasOddRow(8, 4));
    REQUIRE(EXRLoader::pizHasOddRow(6, 4));
    // The two tests agree on the same number, since both are the same bit asked of it.
    for (std::uint32_t n = 0; n < 20; ++n) {
        REQUIRE(EXRLoader::pizHasOddRow(n, 4) == EXRLoader::pizHasOddColumn(n, 4));
    }
}

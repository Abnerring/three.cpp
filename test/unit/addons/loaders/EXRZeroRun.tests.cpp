#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader names the three kinds of symbol") {
    // Below the group a symbol is a real code length.
    REQUIRE(EXRLoader::zeroRunKind(0) == EXRLoader::ZeroRun::None);
    REQUIRE(EXRLoader::zeroRunKind(1) == EXRLoader::ZeroRun::None);
    REQUIRE(EXRLoader::zeroRunKind(58) == EXRLoader::ZeroRun::None);
    // The four values from the first of the group are short runs.
    REQUIRE(EXRLoader::zeroRunKind(59) == EXRLoader::ZeroRun::Short);
    REQUIRE(EXRLoader::zeroRunKind(60) == EXRLoader::ZeroRun::Short);
    REQUIRE(EXRLoader::zeroRunKind(62) == EXRLoader::ZeroRun::Short);
    // The last value alone is a long run.
    REQUIRE(EXRLoader::zeroRunKind(63) == EXRLoader::ZeroRun::Long);
}

TEST_CASE("EXRLoader counts a short zero run") {
    // The first of the group stands for two absent codes, not one.
    REQUIRE(EXRLoader::shortZeroRunLength(59) == 2);
    REQUIRE(EXRLoader::shortZeroRunLength(60) == 3);
    REQUIRE(EXRLoader::shortZeroRunLength(61) == 4);
    REQUIRE(EXRLoader::shortZeroRunLength(62) == 5);
}

TEST_CASE("EXRLoader counts a long zero run") {
    // The eight bits that follow count from six.
    REQUIRE(EXRLoader::longZeroRunLength(0) == 6);
    REQUIRE(EXRLoader::longZeroRunLength(1) == 7);
    REQUIRE(EXRLoader::longZeroRunLength(255) == 261);
    // The shortest long run is one longer than the longest short one, so the two kinds meet
    // without a gap.
    REQUIRE(EXRLoader::longZeroRunLength(0) == EXRLoader::shortZeroRunLength(62) + 1);
}

TEST_CASE("EXRLoader fixes the three constants") {
    REQUIRE(EXRLoader::kShortZeroCodeRun == 59);
    REQUIRE(EXRLoader::kLongZeroCodeRun == 63);
    REQUIRE(EXRLoader::kShortestLongRun == 6);
    // The shortest long run is derived from the two values rather than written out.
    REQUIRE(EXRLoader::kShortestLongRun ==
            2 + EXRLoader::kLongZeroCodeRun - EXRLoader::kShortZeroCodeRun);
}
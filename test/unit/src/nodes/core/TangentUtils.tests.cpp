#include "nodes/core/TangentUtils.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("TangentUtils takes the larger of the two squared lengths") {
    // The determinant is whichever of the two is larger, not their product and not their sum.
    REQUIRE(TangentUtils::determinant(4.0, 9.0) == 9.0);
    REQUIRE(TangentUtils::determinant(9.0, 4.0) == 9.0);
    REQUIRE(TangentUtils::determinant(4.0, 4.0) == 4.0);
    // The reference's form would give sixteen here from a product, so this comparison separates
    // the two.
    REQUIRE(TangentUtils::determinant(4.0, 4.0) != 4.0 * 4.0);
}

TEST_CASE("TangentUtils scales by the inverse square root") {
    // A determinant of four gives one half, since the inverse square root of four is one half.
    REQUIRE(TangentUtils::scale(4.0, 1.0) == 0.5);
    // A determinant of nine gives one third.
    REQUIRE(TangentUtils::scale(1.0, 9.0) == Approx(1.0 / 3.0).epsilon(1e-12));
    // The scale is always the inverse square root of whichever length is larger.
    REQUIRE(TangentUtils::scale(16.0, 1.0) == 0.25);
    REQUIRE(TangentUtils::scale(1.0, 16.0) == 0.25);
}

TEST_CASE("TangentUtils gives no scale to a degenerate surface") {
    // Both lengths nought is degenerate and gives nought rather than an infinity.
    REQUIRE(TangentUtils::degenerate(0.0));
    REQUIRE(TangentUtils::scale(0.0, 0.0) == 0.0);
    // The result is finite, which is the point of the branch.
    REQUIRE(std::isfinite(TangentUtils::scale(0.0, 0.0)));
    // One side nought with the other positive is not degenerate, since the larger is positive.
    REQUIRE_FALSE(TangentUtils::degenerate(TangentUtils::determinant(0.0, 4.0)));
    REQUIRE(TangentUtils::scale(0.0, 4.0) == 0.5);
}

TEST_CASE("TangentUtils treats only exactly nought as degenerate") {
    // A tiny but non zero determinant still gets a scale, however large.
    REQUIRE_FALSE(TangentUtils::degenerate(1e-300));
    REQUIRE(std::isfinite(TangentUtils::scale(1e-300, 0.0)));
    REQUIRE(TangentUtils::scale(1e-300, 0.0) > 0.0);
}
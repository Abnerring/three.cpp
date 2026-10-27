#include "nodes/display/ColorSpaceFunctions.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("ColorSpaceFunctions' exponent going back is a truncation") {
    // The true reciprocal differs from the constant in the sixth decimal place, so the two
    // directions are not exact inverses.
    REQUIRE_FALSE(ColorSpaceFunctions::exponentsAreReciprocal());
    // The reciprocal is the longer of the two, which is what truncating means.
    REQUIRE(1.0 / ColorSpaceFunctions::kEotfExponent > ColorSpaceFunctions::kOetfExponent);
    // The difference is small, so writing the longer value would have been easy to do.
    REQUIRE(std::fabs(1.0 / ColorSpaceFunctions::kEotfExponent -
                      ColorSpaceFunctions::kOetfExponent) < 1e-5);
    REQUIRE(std::fabs(1.0 / ColorSpaceFunctions::kEotfExponent -
                      ColorSpaceFunctions::kOetfExponent) > 0.0);
}

TEST_CASE("ColorSpaceFunctions mixes with the line piece on the flag") {
    // A flag of nought gives the first piece and a flag of one the second.
    REQUIRE(ColorSpaceFunctions::mix(10.0, 20.0, 0.0) == 10.0);
    REQUIRE(ColorSpaceFunctions::mix(10.0, 20.0, 1.0) == 20.0);
    // Halfway is halfway.
    REQUIRE(ColorSpaceFunctions::mix(10.0, 20.0, 0.5) == 15.0);
    // The first piece is the power one, so a small colour takes the straight line rather than
    // the power; this is what the order of the two arguments decides.
    REQUIRE(ColorSpaceFunctions::mix(0.9, 0.1, 1.0) == 0.1);
}

TEST_CASE("ColorSpaceFunctions follows the straight line for small colours") {
    // Below the threshold the flag is one, so the straight line is chosen.
    REQUIRE(ColorSpaceFunctions::eotfFactor(0.04) == 1.0);
    REQUIRE(ColorSpaceFunctions::eotfFactor(0.05) == 0.0);
    // The threshold itself is inclusive, which is what "less than or equal" means.
    REQUIRE(ColorSpaceFunctions::eotfFactor(ColorSpaceFunctions::kEotfThreshold) == 1.0);
    // The same going the other way with its own threshold.
    REQUIRE(ColorSpaceFunctions::oetfFactor(0.003) == 1.0);
    REQUIRE(ColorSpaceFunctions::oetfFactor(0.004) == 0.0);
    REQUIRE(ColorSpaceFunctions::oetfFactor(ColorSpaceFunctions::kOetfThreshold) == 1.0);
}

TEST_CASE("ColorSpaceFunctions converts a small colour by the straight line") {
    // A small colour takes the line, so the answer is the slope times the value.
    REQUIRE(ColorSpaceFunctions::eotf(0.02) == Approx(0.02 * 0.0773993808).epsilon(1e-12));
    // And nothing else, which the threshold satisfies too.
    REQUIRE(ColorSpaceFunctions::eotf(0.0) == 0.0);
    // Going back, the line's slope is the reciprocal, so a tiny value grows.
    REQUIRE(ColorSpaceFunctions::oetf(0.001) == Approx(0.001 * 12.92).epsilon(1e-12));
}

TEST_CASE("ColorSpaceFunctions converts a large colour by the power piece") {
    // A colour above the threshold takes the power piece, whose form differs each way.
    const double forward = std::pow(0.5 * 0.9478672986 + 0.0521327014, 2.4);
    REQUIRE(ColorSpaceFunctions::eotf(0.5) == Approx(forward).epsilon(1e-12));
    const double back = std::pow(0.5, 0.41666) * 1.055 - 0.055;
    REQUIRE(ColorSpaceFunctions::oetf(0.5) == Approx(back).epsilon(1e-12));
    // The two pieces are not the same at the threshold, which is why the flag matters.
    REQUIRE(ColorSpaceFunctions::eotf(0.5) != Approx(0.5 * 0.0773993808).epsilon(1e-12));
}
#include "nodes/core/ColorAdjustment.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("ColorAdjustment's saturation leaves the colour alone at one") {
    // The adjustment is the mix parameter, so one gives the colour back and nought gives the
    // luminance; a test that assumed nought was neutral would be wrong the other way round.
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 1.0) == Approx(0.8).epsilon(1e-12));
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 0.0) == Approx(0.4).epsilon(1e-12));
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 1.0) != Approx(0.4).epsilon(1e-12));
    // Halfway between.
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 0.5) == Approx(0.6).epsilon(1e-12));
}

TEST_CASE("ColorAdjustment's saturation pushes past the colour above one") {
    // Twice the colour less the luminance, which is past the colour itself; this is what super
    // saturating means and it is why the result can exceed the input.
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 2.0) == Approx(1.2).epsilon(1e-12));
    REQUIRE(ColorAdjustment::saturation(0.8, 0.4, 2.0) > 0.8);
    // The result is held at nought from below rather than allowed to go negative.
    REQUIRE(ColorAdjustment::saturation(0.1, 0.5, 3.0) == 0.0);
    REQUIRE(ColorAdjustment::saturation(0.1, 0.5, 3.0) >= 0.0);
}

TEST_CASE("ColorAdjustment's vibrance multiplies by minus three") {
    // A colour whose parts are all equal has no distance from the average, so nothing happens
    // whatever the adjustment.
    const auto flat = ColorAdjustment::vibranceAmounts(0.5, 0.5, 0.5, 1.0);
    REQUIRE(flat.average == Approx(0.5).epsilon(1e-12));
    REQUIRE(flat.maxPart == Approx(0.5).epsilon(1e-12));
    REQUIRE(flat.amount == Approx(0.0).margin(1e-12));
    // A colour with a spread gives a non zero amount, and a positive adjustment makes it
    // negative because of the minus three.
    const auto spread = ColorAdjustment::vibranceAmounts(0.9, 0.3, 0.3, 1.0);
    REQUIRE(spread.maxPart == Approx(0.9).epsilon(1e-12));
    REQUIRE(spread.average == Approx(0.5).epsilon(1e-12));
    REQUIRE(spread.amount == Approx((0.9 - 0.5) * 1.0 * -3.0).epsilon(1e-12));
    REQUIRE(spread.amount < 0.0);
    // With no adjustment the amount is nought however spread the colour is.
    const auto none = ColorAdjustment::vibranceAmounts(0.9, 0.3, 0.3, 0.0);
    REQUIRE(none.amount == Approx(0.0).margin(1e-12));
}

TEST_CASE("ColorAdjustment's vibrance mix moves towards the largest part") {
    // A negative amount pulls the colour the other way, which is what the minus three arranges.
    REQUIRE(ColorAdjustment::vibranceMix(0.5, 0.9, -1.0) == Approx(0.1).epsilon(1e-12));
    // A nought amount leaves the colour alone.
    REQUIRE(ColorAdjustment::vibranceMix(0.5, 0.9, 0.0) == Approx(0.5).epsilon(1e-12));
    // And the result is held at nought.
    REQUIRE(ColorAdjustment::vibranceMix(0.1, 0.9, -2.0) == 0.0);
}

TEST_CASE("ColorAdjustment's hue axis is shorter than the exact value") {
    // The constant is five decimal places of one over the root of three, so the axis it builds
    // is not quite of unit length.
    REQUIRE_FALSE(ColorAdjustment::hueAxisIsExact());
    REQUIRE(ColorAdjustment::kHueAxis == 0.57735);
    // The exact value is the longer of the two, which is what truncating means.
    REQUIRE(1.0 / std::sqrt(3.0) > ColorAdjustment::kHueAxis);
    REQUIRE(std::fabs(1.0 / std::sqrt(3.0) - ColorAdjustment::kHueAxis) < 1e-5);
    REQUIRE(std::fabs(1.0 / std::sqrt(3.0) - ColorAdjustment::kHueAxis) > 0.0);
}
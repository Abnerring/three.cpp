#include "nodes/core/BlendModes.h"

#include "catch.hpp"

TEST_CASE("BlendModes' colour burn darkens and leaves a white blend alone") {
    // A white blend leaves the base alone, which the documentation states and the arithmetic
    // gives: the quotient is one, the clamp keeps it, and inverting twice returns the base.
    REQUIRE(BlendModes::burn(0.5, 1.0) == Approx(0.5).epsilon(1e-12));
    // A black blend sends the quotient to infinity, which clamps to one, giving a black answer.
    REQUIRE(BlendModes::burn(0.5, 0.0) == Approx(0.0).margin(1e-12));
    // Half and half: the quotient is one, so the answer is nought.
    REQUIRE(BlendModes::burn(0.5, 0.5) == Approx(0.0).margin(1e-12));
    // A blend of a quarter divides the inverted base by a quarter, quadrupling it, so the clamp
    // takes over and the answer is still nought.
    REQUIRE(BlendModes::burn(0.5, 0.25) == Approx(0.0).margin(1e-12));
    // A blend near one leaves the base nearly alone.
    REQUIRE(BlendModes::burn(0.5, 0.9) == Approx(1.0 - 0.5 / 0.9).epsilon(1e-12));
}

TEST_CASE("BlendModes' colour dodge lightens and leaves a black blend alone") {
    // A black blend leaves the base alone, since the divisor is one.
    REQUIRE(BlendModes::dodge(0.5, 0.0) == Approx(0.5).epsilon(1e-12));
    // A white blend divides by nought, which clamps to one.
    REQUIRE(BlendModes::dodge(0.5, 1.0) == Approx(1.0).margin(1e-12));
    // Half and half: the divisor is a half, so the quotient is one and the clamp keeps it.
    REQUIRE(BlendModes::dodge(0.5, 0.5) == Approx(1.0).margin(1e-12));
    // A quarter blend doubles the base, which still clamps to one here.
    REQUIRE(BlendModes::dodge(0.25, 0.5) == Approx(0.5).epsilon(1e-12));
}

TEST_CASE("BlendModes' screen lightens less sharply than dodge") {
    // A black blend leaves the base alone, and a white one gives white.
    REQUIRE(BlendModes::screen(0.5, 0.0) == Approx(0.5).epsilon(1e-12));
    REQUIRE(BlendModes::screen(0.5, 1.0) == Approx(1.0).epsilon(1e-12));
    // Half and half gives three quarters, since the product of the inverted halves is a quarter.
    REQUIRE(BlendModes::screen(0.5, 0.5) == Approx(0.75).epsilon(1e-12));
    // At the same inputs screen is below dodge here, which is the "less sharply" the
    // documentation claims.
    REQUIRE(BlendModes::screen(0.5, 0.5) < BlendModes::dodge(0.5, 0.5));
}

TEST_CASE("BlendModes' three modes differ from one another") {
    // With the same pair of colours the three give three different answers, so none of them is
    // another by accident.
    const double burn = BlendModes::burn(0.5, 0.5);
    const double dodge = BlendModes::dodge(0.5, 0.5);
    const double screen = BlendModes::screen(0.5, 0.5);
    REQUIRE(burn != Approx(dodge).epsilon(1e-12));
    REQUIRE(dodge != Approx(screen).epsilon(1e-12));
    REQUIRE(burn != Approx(screen).epsilon(1e-12));
    // Burn darkens below the base and the other two lighten above it.
    REQUIRE(burn < 0.5);
    REQUIRE(dodge > 0.5);
    REQUIRE(screen > 0.5);
}
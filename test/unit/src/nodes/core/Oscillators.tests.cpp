#include "nodes/core/Oscillators.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("Oscillators' fractional part keeps a negative input inside the range") {
    // A positive input gives its own remainder.
    REQUIRE(Oscillators::fract(0.25) == 0.25);
    REQUIRE(Oscillators::fract(1.25) == 0.25);
    // A whole number gives nought.
    REQUIRE(Oscillators::fract(2.0) == 0.0);
    // A negative input gives a value inside the range rather than one with a sign, which is
    // what makes this a fractional part and not a remainder.
    REQUIRE(Oscillators::fract(-0.25) == Approx(0.75).epsilon(1e-12));
    REQUIRE(Oscillators::fract(-1.25) == Approx(0.75).epsilon(1e-12));
    // The result is never negative, which a remainder would not promise.
    REQUIRE(Oscillators::fract(-0.25) > 0.0);
}

TEST_CASE("Oscillators' sine runs one cycle in a unit of time") {
    // The wave adds three quarters of a turn before its angle, so at nought the angle is three
    // quarters of a turn, whose sine is minus one, and the halving and shifting bring it to
    // nought. A quarter later the angle is one turn and the result a half; a half later it is
    // one and a quarter turns, giving one; three quarters later it is back to a half.
    REQUIRE(Oscillators::sine(0.0) == Approx(0.0).margin(1e-12));
    REQUIRE(Oscillators::sine(0.25) == Approx(0.5).epsilon(1e-12));
    REQUIRE(Oscillators::sine(0.5) == Approx(1.0).epsilon(1e-12));
    REQUIRE(Oscillators::sine(0.75) == Approx(0.5).epsilon(1e-12));
    // Without the three quarters it would start at a half rather than at nought.
    REQUIRE(Oscillators::sine(0.0) != 0.5);
}

TEST_CASE("Oscillators' square steps at a half") {
    // Below a half the fractional part rounds down and above it rounds up.
    REQUIRE(Oscillators::square(0.25) == 0.0);
    REQUIRE(Oscillators::square(0.75) == 1.0);
    // Exactly a half rounds up, which is where rounding and a step written the other way round
    // would differ.
    REQUIRE(Oscillators::square(0.5) == 1.0);
    // Whole numbers are on the low side, since their fractional part is nought.
    REQUIRE(Oscillators::square(1.0) == 0.0);
    REQUIRE(Oscillators::square(1.25) == 0.0);
    REQUIRE(Oscillators::square(1.75) == 1.0);
}

TEST_CASE("Oscillators' triangle folds") {
    // The half it adds puts the fractional part at a half at nought, which doubles and shifts
    // to nought before the fold; a quarter gives a half, a half gives one, and three
    // quarters come back to a half. The wave therefore reaches its top once per cycle and
    // its ends are nought at the start and a half at that last point.
    REQUIRE(Oscillators::triangle(0.0) == Approx(0.0).margin(1e-12));
    REQUIRE(Oscillators::triangle(0.25) == Approx(0.5).epsilon(1e-12));
    REQUIRE(Oscillators::triangle(0.5) == Approx(1.0).epsilon(1e-12));
    REQUIRE(Oscillators::triangle(0.75) == Approx(0.5).epsilon(1e-12));
    // It never goes below nought, which is what the fold guarantees.
    for (int i = 0; i <= 20; ++i) {
        REQUIRE(Oscillators::triangle(i / 20.0) >= 0.0);
    }
}

TEST_CASE("Oscillators' sawtooth is the fractional part itself") {
    REQUIRE(Oscillators::sawtooth(0.0) == 0.0);
    REQUIRE(Oscillators::sawtooth(0.25) == 0.25);
    REQUIRE(Oscillators::sawtooth(0.99) == 0.99);
    // It wraps at a whole number rather than continuing.
    REQUIRE(Oscillators::sawtooth(1.0) == 0.0);
    REQUIRE(Oscillators::sawtooth(1.25) == 0.25);
    // The two constants are the offsets the first and third waves carry.
    REQUIRE(Oscillators::kSineOffset == 0.75);
    REQUIRE(Oscillators::kTriangleOffset == 0.5);
}

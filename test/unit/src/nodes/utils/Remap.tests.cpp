#include "nodes/utils/Remap.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("Remap converts the documented example") {
    // The documentation says a value of nought point four in the range nought point three to
    // nought point five becomes a half in the range nought to one.
    const double t = (0.4 - 0.3) / (0.5 - 0.3);
    REQUIRE(t == Approx(0.5).epsilon(1e-12));
    REQUIRE(Remap::value(0.4, 0.3, 0.5, 0.0, 1.0) == Approx(0.5).epsilon(1e-12));
    // The low end of the input range becomes the low end of the output.
    REQUIRE(Remap::value(0.3, 0.3, 0.5, 0.0, 1.0) == Approx(0.0).margin(1e-12));
    // And the high end becomes the high end.
    REQUIRE(Remap::value(0.5, 0.3, 0.5, 0.0, 1.0) == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("Remap stretches into an output range that is not nought to one") {
    // A half in the input becomes the middle of any output range.
    REQUIRE(Remap::value(0.4, 0.3, 0.5, 10.0, 20.0) == Approx(15.0).epsilon(1e-12));
    // A tenth of the way in becomes a tenth of the way through the output.
    REQUIRE(Remap::value(0.32, 0.3, 0.5, 0.0, 100.0) == Approx(10.0).margin(1e-9));
    // An output range that runs backwards is allowed and reverses the mapping.
    REQUIRE(Remap::value(0.3, 0.3, 0.5, 1.0, 0.0) == Approx(1.0).epsilon(1e-12));
    REQUIRE(Remap::value(0.5, 0.3, 0.5, 1.0, 0.0) == Approx(0.0).margin(1e-12));
}

TEST_CASE("Remap's clamp changes only values outside the input range") {
    // Inside the range the two agree.
    for (int i = 0; i <= 5; ++i) {
        const double v = 0.3 + i * 0.04;
        REQUIRE(Remap::value(v, 0.3, 0.5, 0.0, 1.0) ==
                Approx(Remap::clamped(v, 0.3, 0.5, 0.0, 1.0)).margin(1e-12));
    }
    // Below the range the clamped one stops at the low end while the other goes past it.
    REQUIRE(Remap::value(0.2, 0.3, 0.5, 0.0, 1.0) == Approx(-0.5).margin(1e-12));
    REQUIRE(Remap::clamped(0.2, 0.3, 0.5, 0.0, 1.0) == Approx(0.0).margin(1e-12));
    // And above it at the high end.
    REQUIRE(Remap::value(0.6, 0.3, 0.5, 0.0, 1.0) == Approx(1.5).margin(1e-12));
    REQUIRE(Remap::clamped(0.6, 0.3, 0.5, 0.0, 1.0) == Approx(1.0).margin(1e-12));
}

TEST_CASE("Remap's clamp step uses the result of TSL defined") {
    REQUIRE(Remap::runsClampStep(true));
    // defined(bool(false)) evaluates to false; presence alone does not clamp.
    REQUIRE(Remap::runsClampStep(true) != Remap::runsClampStep(false));
    REQUIRE_FALSE(Remap::runsClampStep(false));
}

TEST_CASE("Remap does not guard a zero width input range") {
    // A range with no width divides by nought, so the value is infinite rather than held.
    // The numerator here is the value less the low end, which is minus a tenth, and the
    // denominator is plus nought, so the answer is minus infinity rather than plus; I had
    // written the sign the wrong way round and this assertion records the corrected one.
    const double out = Remap::value(0.4, 0.5, 0.5, 0.0, 1.0);
    REQUIRE(std::isinf(out));
    REQUIRE(out < 0.0);
    // And the clamped form stops at the low end rather than reporting the infinity, since
    // the normalised value was minus infinity: the sign of the numerator decides which end
    // of the range the clamp lands on, and here it is the low one.
    REQUIRE(Remap::clamped(0.4, 0.5, 0.5, 0.0, 1.0) == Approx(0.0).margin(1e-12));
    // A numerator on the other side would land on the high end, which shows the sign is
    // what decides and not the zero width itself.
    REQUIRE(Remap::clamped(0.6, 0.5, 0.5, 0.0, 1.0) == Approx(1.0).margin(1e-12));
}

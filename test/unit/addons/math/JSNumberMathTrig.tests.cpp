#include "catch.hpp"
#include "math/JSNumberMath.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

// Exact identities of the tan/asin/atan/atan2 entry points added to the pinned Node/V8 fdlibm
// port in src/math/JSNumberMath.cpp. compare-math-trig.mjs compares the bit patterns in bulk
// against the pinned Node release; the identities below are the ones worth stating as
// properties, in particular the sign of zero that Math.atan2 defines.
namespace {
// The doubles the pinned Node release returns for the branch results quoted here, so the
// comparisons stay exact instead of approximate. Math.asin(1) is pio2_hi + pio2_lo, which
// rounds to this same double.
const double PI = 3.141592653589793;          // 0x400921FB54442D18
const double HALF_PI = 1.5707963267948966;    // 0x3FF921FB54442D18
const double QUARTER_PI = 0.7853981633974483; // 0x3FE921FB54442D18
}

TEST_CASE("JSNumberMath.atan2 keeps the sign of zero and the ES quadrant rules", "[jsnumbermathtrig]") {
    REQUIRE(JSNumberMath::atan2(0.0, 1.0) == 0.0);
    REQUIRE(!std::signbit(JSNumberMath::atan2(0.0, 1.0)));
    REQUIRE(JSNumberMath::atan2(-0.0, 1.0) == 0.0);
    REQUIRE(std::signbit(JSNumberMath::atan2(-0.0, 1.0)));
    REQUIRE(JSNumberMath::atan2(0.0, 0.0) == 0.0);
    REQUIRE(JSNumberMath::atan2(0.0, -0.0) == PI);
    REQUIRE(JSNumberMath::atan2(-0.0, -0.0) == -PI);
    REQUIRE(JSNumberMath::atan2(1.0, 1.0) == QUARTER_PI);
    REQUIRE(JSNumberMath::atan2(-1.0, 1.0) == -QUARTER_PI);
    REQUIRE(JSNumberMath::atan2(1.0, -1.0) == 3.0 * QUARTER_PI);
    REQUIRE(JSNumberMath::atan2(std::numeric_limits<double>::infinity(), 1.0) == HALF_PI);
    REQUIRE(JSNumberMath::atan2(1.0, std::numeric_limits<double>::infinity()) == 0.0);
    REQUIRE(std::signbit(JSNumberMath::atan2(-1.0, std::numeric_limits<double>::infinity())));
    REQUIRE(JSNumberMath::atan2(std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::infinity()) == QUARTER_PI);
    REQUIRE(std::isnan(JSNumberMath::atan2(std::numeric_limits<double>::quiet_NaN(), 1.0)));
    REQUIRE(std::isnan(JSNumberMath::atan2(1.0, std::numeric_limits<double>::quiet_NaN())));
    // A NaN whose high word is exactly 0x7FF00000 leaves the upstream NaN test with only the
    // low word to look at, so the ported two's-complement wraparound helper has to classify it
    // as NaN rather than as an infinity.
    const double lowWordNaN = std::bit_cast<double>(std::uint64_t{0x7FF0000000000001});
    REQUIRE(std::isnan(JSNumberMath::atan2(1.0, lowWordNaN)));
    REQUIRE(std::isnan(JSNumberMath::atan2(lowWordNaN, 1.0)));
}

TEST_CASE("JSNumberMath.asin matches the pinned branch results", "[jsnumbermathtrig]") {
    REQUIRE(JSNumberMath::asin(0.0) == 0.0);
    REQUIRE(std::signbit(JSNumberMath::asin(-0.0)));
    REQUIRE(JSNumberMath::asin(1.0) == HALF_PI);
    REQUIRE(JSNumberMath::asin(-1.0) == -HALF_PI);
    REQUIRE(JSNumberMath::asin(0.5) == 0.5235987755982989);           // pi/6
    REQUIRE(JSNumberMath::asin(0.9999999999999999) == 1.5707963118937354);
    // |x| > 1 is NaN, and a NaN input stays NaN.
    REQUIRE(std::isnan(JSNumberMath::asin(2.0)));
    REQUIRE(std::isnan(JSNumberMath::asin(-1.0000000000000002)));
    REQUIRE(std::isnan(JSNumberMath::asin(std::numeric_limits<double>::infinity())));
    REQUIRE(std::isnan(JSNumberMath::asin(std::numeric_limits<double>::quiet_NaN())));
}

TEST_CASE("JSNumberMath.atan matches the pinned branch results", "[jsnumbermathtrig]") {
    REQUIRE(JSNumberMath::atan(0.0) == 0.0);
    REQUIRE(std::signbit(JSNumberMath::atan(-0.0)));
    REQUIRE(JSNumberMath::atan(1.0) == QUARTER_PI);
    REQUIRE(JSNumberMath::atan(-1.0) == -QUARTER_PI);
    REQUIRE(JSNumberMath::atan(std::numeric_limits<double>::infinity()) == HALF_PI);
    REQUIRE(JSNumberMath::atan(-std::numeric_limits<double>::infinity()) == -HALF_PI);
    REQUIRE(std::isnan(JSNumberMath::atan(std::numeric_limits<double>::quiet_NaN())));
}

TEST_CASE("JSNumberMath.tan matches the pinned branch results", "[jsnumbermathtrig]") {
    REQUIRE(JSNumberMath::tan(0.0) == 0.0);
    REQUIRE(std::signbit(JSNumberMath::tan(-0.0)));
    REQUIRE(JSNumberMath::tan(QUARTER_PI) == 0.9999999999999999);
    REQUIRE(JSNumberMath::tan(-QUARTER_PI) == -0.9999999999999999);
    // The reduced argument at the double nearest pi/2 is tiny, so the result is the huge
    // finite value Math.tan( 1.5707963267948966 ) returns rather than an infinity.
    REQUIRE(JSNumberMath::tan(HALF_PI) == 1.633123935319537e16);
    REQUIRE(std::isnan(JSNumberMath::tan(std::numeric_limits<double>::infinity())));
    REQUIRE(std::isnan(JSNumberMath::tan(-std::numeric_limits<double>::infinity())));
    REQUIRE(std::isnan(JSNumberMath::tan(std::numeric_limits<double>::quiet_NaN())));
}

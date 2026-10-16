#include "catch.hpp"
#include "math/JSNumberMath.h"
#include <cmath>
#include <limits>

// Exact identities of the exp/log/log2 entry points added to the pinned Node/V8 fdlibm port in
// src/math/JSNumberMath.cpp. compare-math-exp-log.mjs compares the bit patterns in bulk against
// the pinned Node release; the identities below are the ones worth stating as properties, in
// particular the direction of the infinities and the sign of zero that ES defines.
namespace {
// The doubles the pinned Node release returns for the values quoted here, so the comparisons
// stay exact instead of approximate.
const double E = 2.718281828459045;    // 0x4005BF0A8B145769, Math.exp(1)
const double LN2 = 0.6931471805599453; // 0x3FE62E42FEFA39EF, Math.log(2)
const double LN10 = 2.302585092994046; // 0x40026BB1BBB55516, Math.log(10)
}

TEST_CASE("JSNumberMath.exp matches the pinned branch results", "[jsnumbermathexplog]") {
    // exp(0) and exp(-0) are both exactly 1, and exp(1) is the value of the upstream special
    // case rather than the polynomial's last-bit neighbour.
    REQUIRE(JSNumberMath::exp(0.0) == 1.0);
    REQUIRE(JSNumberMath::exp(-0.0) == 1.0);
    REQUIRE(JSNumberMath::exp(1.0) == E);
    // The underflow path returns +0, and exp(+-Infinity) is {Infinity, +0} per the sign of x.
    REQUIRE(JSNumberMath::exp(-745.2) == 0.0);
    REQUIRE(!std::signbit(JSNumberMath::exp(-745.2)));
    REQUIRE(JSNumberMath::exp(-std::numeric_limits<double>::infinity()) == 0.0);
    REQUIRE(!std::signbit(JSNumberMath::exp(-std::numeric_limits<double>::infinity())));
    REQUIRE(std::isinf(JSNumberMath::exp(std::numeric_limits<double>::infinity())));
    REQUIRE(JSNumberMath::exp(710.0) == std::numeric_limits<double>::infinity());
    REQUIRE(JSNumberMath::exp(1e300) == std::numeric_limits<double>::infinity());
    REQUIRE(std::isnan(JSNumberMath::exp(std::numeric_limits<double>::quiet_NaN())));
}

TEST_CASE("JSNumberMath.log matches the pinned branch results", "[jsnumbermathexplog]") {
    // log(1) is the exact +0 of the upstream fast path, not -0.
    REQUIRE(JSNumberMath::log(1.0) == 0.0);
    REQUIRE(!std::signbit(JSNumberMath::log(1.0)));
    REQUIRE(JSNumberMath::log(E) == 1.0);
    REQUIRE(JSNumberMath::log(2.0) == LN2);
    REQUIRE(JSNumberMath::log(10.0) == LN10);
    // The two doubles next to 1 exercise the -2**-20 <= f < 2**-20 branch, where the result is
    // built from the correction term rather than from the polynomial kernel.
    REQUIRE(JSNumberMath::log(1.0000000000000002) == 2.2204460492503128e-16);
    REQUIRE(JSNumberMath::log(0.9999999999999999) == -1.1102230246251565e-16);
    // log(+-0) is -Infinity with the sign bit set; a negative argument is NaN.
    REQUIRE(std::isinf(JSNumberMath::log(0.0)));
    REQUIRE(std::signbit(JSNumberMath::log(0.0)));
    REQUIRE(std::isinf(JSNumberMath::log(-0.0)));
    REQUIRE(std::signbit(JSNumberMath::log(-0.0)));
    REQUIRE(std::isnan(JSNumberMath::log(-1.0)));
    REQUIRE(std::isnan(JSNumberMath::log(-5e-324)));
    REQUIRE(std::isnan(JSNumberMath::log(std::numeric_limits<double>::quiet_NaN())));
    REQUIRE(std::isinf(JSNumberMath::log(std::numeric_limits<double>::infinity())));
    // The subnormal path scales by 2**54 before reducing, and the huge end stays finite.
    REQUIRE(JSNumberMath::log(5e-324) == -744.4400719213812);
    REQUIRE(JSNumberMath::log(1e-320) == -736.8272408909739);
    REQUIRE(JSNumberMath::log(1e308) == 709.1962086421661);
    REQUIRE(JSNumberMath::log(std::numeric_limits<double>::max()) == 709.782712893384);
}

TEST_CASE("JSNumberMath.log2 matches the pinned branch results", "[jsnumbermathexplog]") {
    // log2(1) returns the exact +0 of the upstream shortcut before any reduction happens.
    REQUIRE(JSNumberMath::log2(1.0) == 0.0);
    REQUIRE(!std::signbit(JSNumberMath::log2(1.0)));
    REQUIRE(JSNumberMath::log2(2.0) == 1.0);
    REQUIRE(JSNumberMath::log2(4.0) == 2.0);
    REQUIRE(JSNumberMath::log2(0.5) == -1.0);
    REQUIRE(JSNumberMath::log2(1024.0) == 10.0);
    REQUIRE(JSNumberMath::log2(std::numeric_limits<double>::max()) == 1024.0);
    // The subnormal path scales by 2**54, so the exponent of the smallest subnormal is exact.
    REQUIRE(JSNumberMath::log2(5e-324) == -1074.0);
    REQUIRE(JSNumberMath::log2(1e300) == 996.5784284662087);
    REQUIRE(JSNumberMath::log2(1e-300) == -996.5784284662087);
    REQUIRE(std::isinf(JSNumberMath::log2(0.0)));
    REQUIRE(std::signbit(JSNumberMath::log2(0.0)));
    REQUIRE(std::isinf(JSNumberMath::log2(-0.0)));
    REQUIRE(std::signbit(JSNumberMath::log2(-0.0)));
    REQUIRE(std::isnan(JSNumberMath::log2(-1.0)));
    REQUIRE(std::isnan(JSNumberMath::log2(std::numeric_limits<double>::quiet_NaN())));
    REQUIRE(std::isinf(JSNumberMath::log2(std::numeric_limits<double>::infinity())));
}

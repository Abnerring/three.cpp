#include "catch.hpp"
#include "geometries/ParametricFunctions.h"
#include "math/Vector3.h"
#include <cmath>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/ParametricFunctions.js under the pinned Node oracle.
namespace {
const double epsilon = 1e-9;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("ParametricFunctions plane maps u and v onto x and z", "[parametricfunctions]") {
    Vector3 target;
    ParametricFunctions::plane(0.25, 0.75, target);
    REQUIRE(target.x == 0.25);
    REQUIRE(target.y == 0.);
    REQUIRE(target.z == 0.75);
    ParametricFunctions::plane(0, 1, target);
    REQUIRE(target.x == 0.);
    REQUIRE(target.z == 1.);
}

TEST_CASE("ParametricFunctions klein keeps the upstream parameter swap", "[parametricfunctions]") {
    // The pinned function is declared klein( v, u, target ): the first argument drives the
    // `v` expressions, the second one the `u` expressions. With v = 0.2 the cosine term is
    // cos( 2 * PI * 0.2 ) = cos( 72 degrees ), so the double result is exact to the last bit
    // (the oracle's attribute array carries the float32 rounding of the geometry, not this).
    Vector3 target;
    ParametricFunctions::klein(0.2, 0., target);
    REQUIRE(close(target.x, 3.3090169943749475));
    REQUIRE(close(target.y, -0.9510565162951535));
    REQUIRE(close(target.z, 0.));
    // Swapping the arguments must give a different point, as it does upstream.
    Vector3 swapped;
    ParametricFunctions::klein(0., 0.2, swapped);
    REQUIRE_FALSE(close(swapped.x, target.x));
}

TEST_CASE("ParametricFunctions mobius and mobius3d match the pinned formulas", "[parametricfunctions]") {
    Vector3 target;
    ParametricFunctions::mobius(0.5, 0, target);
    REQUIRE(close(target.x, 2.));
    REQUIRE(std::abs(target.y) <= epsilon);
    ParametricFunctions::mobius(0.25, 0.5, target);
    REQUIRE(close(target.x, -2.));
    ParametricFunctions::mobius3d(0, 0, target);
    REQUIRE(close(target.x, 2.375));
    REQUIRE(close(target.y, 0.));
    REQUIRE(close(target.z, 0.));
    ParametricFunctions::mobius3d(0.5, 0.25, target);
    REQUIRE(std::isfinite(target.x));
    REQUIRE(std::isfinite(target.y));
    REQUIRE(std::isfinite(target.z));
}

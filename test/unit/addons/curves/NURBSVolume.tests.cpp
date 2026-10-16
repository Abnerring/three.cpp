#include "catch.hpp"
#include "core/JSValue.h"
#include "curves/NURBSVolume.h"
#include <cmath>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/curves/NURBSVolume.js under the pinned Node oracle.
namespace {
const std::vector<double> unit{0, 0, 1, 1};
NURBSUtils::ControlVolume unitCube() {
    return {{{Vector4(0, 0, 0, 1), Vector4(0, 1, 0, 1)}, {Vector4(1, 0, 0, 1), Vector4(1, 1, 0, 1)}},
        {{Vector4(0, 0, 1, 1), Vector4(0, 1, 1, 1)}, {Vector4(1, 0, 1, 1), Vector4(1, 1, 1, 1)}}};
}
const double epsilon = 1e-14;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("NURBSVolume copies the block selected by the knot vectors", "[nurbs]") {
    NURBSVolume volume(1, 1, 1, unit, unit, unit, unitCube());
    REQUIRE(volume.degree1 == 1);
    REQUIRE(volume.degree2 == 1);
    REQUIRE(volume.degree3 == 1);
    REQUIRE(volume.controlPoints.size() == 2);
    REQUIRE(volume.controlPoints[0].size() == 2);
    REQUIRE(volume.controlPoints[0][0].size() == 2);
    NURBSUtils::ControlVolume input = unitCube();
    NURBSVolume copied(1, 1, 1, unit, unit, unit, input);
    input[0][0][0].x = 42.;
    REQUIRE(copied.controlPoints[0][0][0].x == 0.);
    // The loops copy knots.length - degree - 1 entries per axis, so a missing point throws.
    NURBSUtils::ControlVolume tooThin{{{{Vector4(0, 0, 0, 1)}, {Vector4(1, 0, 0, 1)}},
        {{Vector4(0, 0, 1, 1)}, {Vector4(1, 0, 1, 1)}}}};
    REQUIRE_THROWS_AS(NURBSVolume(1, 1, 1, unit, unit, unit, tooThin), JSTypeError);
}

TEST_CASE("NURBSVolume getPoint matches the pinned volume", "[nurbs]") {
    NURBSVolume volume(1, 1, 1, unit, unit, unit, unitCube());
    Vector3 target;
    volume.getPoint(0, 0, 0, target);
    REQUIRE(target.x == 0.);
    REQUIRE(target.y == 0.);
    REQUIRE(target.z == 0.);
    volume.getPoint(1, 1, 1, target);
    REQUIRE(target.x == 1.);
    REQUIRE(target.y == 1.);
    REQUIRE(target.z == 1.);
    volume.getPoint(0.5, 0.5, 0.5, target);
    REQUIRE(close(target.x, 0.5));
    REQUIRE(close(target.y, 0.5));
    REQUIRE(close(target.z, 0.5));
    volume.getPoint(0.25, 0.75, 0.5, target);
    REQUIRE(close(target.x, 0.75));
    REQUIRE(close(target.y, 0.5));
    REQUIRE(close(target.z, 0.25));
}

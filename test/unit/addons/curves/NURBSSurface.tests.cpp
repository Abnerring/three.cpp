#include "catch.hpp"
#include "core/JSValue.h"
#include "curves/NURBSSurface.h"
#include <cmath>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/curves/NURBSSurface.js under the pinned Node oracle.
namespace {
const std::vector<double> unit{0, 0, 1, 1};
const NURBSUtils::ControlGrid grid{{Vector4(0, 0, 0, 1), Vector4(0, 1, 0, 2)},
    {Vector4(1, 0, 0, 3), Vector4(1, 1, 0, 4)}};
const double epsilon = 1e-14;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("NURBSSurface copies the block selected by the knot vectors", "[nurbs]") {
    NURBSSurface surface(1, 1, unit, unit, grid);
    REQUIRE(surface.degree1 == 1);
    REQUIRE(surface.degree2 == 1);
    REQUIRE(surface.knots1 == unit);
    REQUIRE(surface.knots2 == unit);
    REQUIRE(surface.controlPoints.size() == 2);
    REQUIRE(surface.controlPoints[0].size() == 2);
    // The upstream constructor rebuilds every point, so later input edits do not reach it.
    NURBSUtils::ControlGrid input = grid;
    NURBSSurface copied(1, 1, unit, unit, input);
    input[0][0].x = 42.;
    REQUIRE(copied.controlPoints[0][0].x == 0.);
    // A higher degree keeps fewer control points: knots.length - degree - 1.
    NURBSUtils::ControlGrid wide;
    for (int row = 0; row < 5; ++row) wide.push_back({Vector4(double(row), 0, 0, 1), Vector4(double(row), 1, 0, 1)});
    NURBSSurface trimmed(2, 1, {0, 0, 0, 1, 2, 3, 3, 3}, unit, wide);
    REQUIRE(trimmed.controlPoints.size() == 5);
    REQUIRE(trimmed.controlPoints[0].size() == 2);
    REQUIRE(trimmed.controlPoints[4][1].y == 1.);
}

TEST_CASE("NURBSSurface getPoint matches the pinned surface", "[nurbs]") {
    NURBSSurface surface(1, 1, unit, unit, grid);
    Vector3 target;
    surface.getPoint(0, 0, target);
    REQUIRE(target.x == 0.);
    REQUIRE(target.y == 0.);
    REQUIRE(target.z == 0.);
    surface.getPoint(1, 1, target);
    REQUIRE(target.x == 1.);
    REQUIRE(target.y == 1.);
    surface.getPoint(0.5, 0.5, target);
    REQUIRE(close(target.x, 0.7000000000000001));
    REQUIRE(close(target.y, 0.6000000000000001));
    surface.getPoint(0.25, 0.75, target);
    REQUIRE(close(target.x, 0.41666666666666663));
    REQUIRE(close(target.y, 0.8333333333333333));
    // A partial control point block keeps the upstream TypeError.
    const NURBSUtils::ControlGrid tooFew{{Vector4(0, 0, 0, 1)}, {Vector4(1, 0, 0, 1)}};
    REQUIRE_THROWS_AS(NURBSSurface(1, 1, unit, unit, tooFew), JSTypeError);
}

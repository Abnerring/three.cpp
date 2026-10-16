#include "catch.hpp"
#include "core/JSValue.h"
#include "curves/NURBSUtils.h"
#include <cmath>
#include <limits>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/curves/NURBSUtils.js under the pinned Node oracle.
namespace {
const std::vector<double> knots{0, 0, 0, 1, 2, 3, 3, 3};
const std::vector<Vector4> control{{0, 0, 0, 1}, {1, 2, 0, 1}, {2, 0, 0, 1}, {3, 1, 0, 1}, {4, 0, 0, 1}};
const double epsilon = 1e-14;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("NURBSUtils findSpan matches the pinned knot search", "[nurbs]") {
    const std::vector<double> parameters{0, 0.5, 1, 1.5, 2, 2.9999, 3};
    const std::vector<double> expected{2, 2, 3, 3, 4, 4, 4};
    for (std::size_t i = 0; i < parameters.size(); ++i)
        REQUIRE(NURBSUtils::findSpan(2, parameters[i], knots) == expected[i]);
    // Values outside the domain clamp onto the first and last span.
    REQUIRE(NURBSUtils::findSpan(2, -5, knots) == 2);
    REQUIRE(NURBSUtils::findSpan(2, 10, knots) == 4);
}

TEST_CASE("NURBSUtils calcBasisFunctions reproduces the pinned basis values", "[nurbs]") {
    const auto first = NURBSUtils::calcBasisFunctions(2, 0.5, 2, knots);
    REQUIRE(first.size() == 3);
    REQUIRE(close(first[0], 0.25));
    REQUIRE(close(first[1], 0.625));
    REQUIRE(close(first[2], 0.125));
    const auto second = NURBSUtils::calcBasisFunctions(3, 1, 2, knots);
    REQUIRE(close(second[0], 0.5));
    REQUIRE(close(second[1], 0.5));
    REQUIRE(close(second[2], 0.));
    // Partition of unity across the domain.
    for (const double u : {0., 0.25, 0.75, 1.3, 2.4, 3.}) {
        const auto basis = NURBSUtils::calcBasisFunctions(NURBSUtils::findSpan(2, u, knots), u, 2, knots);
        double sum = 0;
        for (const auto value : basis) sum += value;
        REQUIRE(close(sum, 1.));
    }
}

TEST_CASE("NURBSUtils calcBSplinePoint matches the pinned homogeneous points", "[nurbs]") {
    const std::vector<double> parameters{0, 0.5, 1, 1.5, 2, 3};
    const std::vector<Vector4> expected{{0, 0, 0, 1}, {0.875, 1.25, 0, 1}, {1.5, 1, 0, 1},
        {2, 0.375, 0, 1}, {2.5, 0.5, 0, 1}, {4, 0, 0, 1}};
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        const Vector4 point = NURBSUtils::calcBSplinePoint(2, knots, control, parameters[i]);
        REQUIRE(close(point.x, expected[i].x));
        REQUIRE(close(point.y, expected[i].y));
        REQUIRE(close(point.z, expected[i].z));
        REQUIRE(close(point.w, expected[i].w));
    }
}

TEST_CASE("NURBSUtils derivative helpers match the pinned results", "[nurbs]") {
    const auto ders = NURBSUtils::calcBasisFunctionDerivatives(3, 1, 2, 2, knots);
    REQUIRE(ders.size() == 3);
    const std::vector<double> flat{0.5, 0.5, 0, -1, 1, 0, 1, -2, 1};
    for (std::size_t row = 0; row < ders.size(); ++row)
        for (std::size_t column = 0; column < ders[row].size(); ++column)
            REQUIRE(close(ders[row][column], flat[row * 3 + column]));
    REQUIRE(close(NURBSUtils::calcKoverI(0, 0), 1.));
    REQUIRE(close(NURBSUtils::calcKoverI(4, 2), 6.));
    REQUIRE(close(NURBSUtils::calcKoverI(6, 3), 20.));
    REQUIRE(close(NURBSUtils::calcKoverI(5, 0), 1.));
    const auto bspline = NURBSUtils::calcBSplineDerivatives(2, knots, control, 1, 2);
    // The upstream zero fill runs to `nd + 1` and uses the default w = 1 for those entries.
    REQUIRE(bspline.size() == 4);
    REQUIRE(close(bspline[0].x, 1.5));
    REQUIRE(close(bspline[0].y, 1.));
    REQUIRE(close(bspline[0].w, 1.));
    REQUIRE(close(bspline[1].x, 1.));
    REQUIRE(close(bspline[1].y, -2.));
    REQUIRE(close(bspline[1].w, 0.));
    REQUIRE(close(bspline[2].x, 0.));
    REQUIRE(close(bspline[2].y, 3.));
    REQUIRE(close(bspline[3].w, 1.));
    const auto nurbs = NURBSUtils::calcNURBSDerivatives(2, knots, control, 1, 1);
    REQUIRE(nurbs.size() == 3);
    REQUIRE(close(nurbs[0].x, 1.5));
    REQUIRE(close(nurbs[0].y, 1.));
    REQUIRE(close(nurbs[1].x, 1.));
    REQUIRE(close(nurbs[1].y, -2.));
    REQUIRE(close(nurbs[2].x, -1.5));
    REQUIRE(close(nurbs[2].y, -1.));
}

TEST_CASE("NURBSUtils surface and volume points match the pinned results", "[nurbs]") {
    const std::vector<double> unit{0, 0, 1, 1};
    const NURBSUtils::ControlGrid grid{{Vector4(0, 0, 0, 1), Vector4(0, 1, 0, 2)},
        {Vector4(1, 0, 0, 3), Vector4(1, 1, 0, 4)}};
    Vector3 target;
    NURBSUtils::calcSurfacePoint(1, 1, unit, unit, grid, 0, 0, target);
    REQUIRE(target.x == 0.);
    REQUIRE(target.y == 0.);
    NURBSUtils::calcSurfacePoint(1, 1, unit, unit, grid, 1, 1, target);
    REQUIRE(target.x == 1.);
    REQUIRE(target.y == 1.);
    NURBSUtils::calcSurfacePoint(1, 1, unit, unit, grid, 0.5, 0.5, target);
    REQUIRE(close(target.x, 0.7000000000000001));
    REQUIRE(close(target.y, 0.6000000000000001));
    NURBSUtils::calcSurfacePoint(1, 1, unit, unit, grid, 0.25, 0.75, target);
    REQUIRE(close(target.x, 0.41666666666666663));
    REQUIRE(close(target.y, 0.8333333333333333));

    const NURBSUtils::ControlVolume volume{
        {{Vector4(0, 0, 0, 1), Vector4(0, 1, 0, 1)}, {Vector4(1, 0, 0, 1), Vector4(1, 1, 0, 1)}},
        {{Vector4(0, 0, 1, 1), Vector4(0, 1, 1, 1)}, {Vector4(1, 0, 1, 1), Vector4(1, 1, 1, 1)}}};
    NURBSUtils::calcVolumePoint(1, 1, 1, unit, unit, unit, volume, 1, 1, 1, target);
    REQUIRE(target.x == 1.);
    REQUIRE(target.y == 1.);
    REQUIRE(target.z == 1.);
    NURBSUtils::calcVolumePoint(1, 1, 1, unit, unit, unit, volume, 0.25, 0.75, 0.5, target);
    REQUIRE(close(target.x, 0.75));
    REQUIRE(close(target.y, 0.5));
    REQUIRE(close(target.z, 0.25));
}

TEST_CASE("NURBSUtils reports the upstream TypeError for a missing control point", "[nurbs]") {
    const std::vector<Vector4> tooFew{{0, 0, 0, 1}, {1, 1, 0, 1}};
    REQUIRE_THROWS_AS(NURBSUtils::calcBSplinePoint(2, knots, tooFew, 2), JSTypeError);
    const NURBSUtils::ControlGrid grid{{Vector4(0, 0, 0, 1)}, {Vector4(1, 0, 0, 1)}};
    Vector3 target;
    REQUIRE_THROWS_AS(NURBSUtils::calcSurfacePoint(1, 1, {0, 0, 1, 1}, {0, 0, 1, 1}, grid, 0.5, 0.5, target),
        JSTypeError);
}

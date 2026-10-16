#include "catch.hpp"
#include "curves/NURBSCurve.h"
#include <cmath>
#include <limits>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/curves/NURBSCurve.js under the pinned Node oracle.
namespace {
const std::vector<double> knots{0, 0, 0, 1, 2, 3, 3, 3};
const std::vector<Vector4> control{{0, 0, 0, 1}, {1, 2, 0, 1}, {2, 0, 0, 1}, {3, 1, 0, 1}, {4, 0, 0, 1}};
const double epsilon = 1e-14;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("NURBSCurve stores the pinned construction state", "[nurbs]") {
    NURBSCurve curve(2, knots, control);
    REQUIRE(curve.degree == 2);
    REQUIRE(curve.knots == knots);
    REQUIRE(curve.controlPoints.size() == 5);
    REQUIRE(curve.startKnot == 0);
    REQUIRE(curve.endKnot == 7);
    // `startKnot || 0` and `endKnot || knotsLength` keep raw numbers when truthy.
    NURBSCurve indexed(2, knots, control, 3, 5);
    REQUIRE(indexed.startKnot == 3);
    REQUIRE(indexed.endKnot == 5);
    NURBSCurve falsy(2, knots, control, 0, std::numeric_limits<double>::quiet_NaN());
    REQUIRE(falsy.startKnot == 0);
    REQUIRE(falsy.endKnot == 7);
    // The upstream constructor rebuilds every control point, so the input stays untouched.
    NURBSCurve copiedInput(2, knots, control);
    copiedInput.controlPoints[0].x = 99.;
    REQUIRE(control[0].x == 0.);
}

TEST_CASE("NURBSCurve getPoint and getTangent match the pinned curve", "[nurbs]") {
    NURBSCurve curve(2, knots, control);
    Vector3 point;
    curve.getPoint(0, point);
    REQUIRE(point.x == 0.);
    REQUIRE(point.y == 0.);
    curve.getPoint(0.25, point);
    REQUIRE(close(point.x, 1.21875));
    REQUIRE(close(point.y, 1.3125));
    curve.getPoint(0.5, point);
    REQUIRE(close(point.x, 2.));
    REQUIRE(close(point.y, 0.375));
    curve.getPoint(1, point);
    REQUIRE(close(point.x, 4.));
    REQUIRE(close(point.y, 0.));
    Vector3 tangent;
    curve.getTangent(0, tangent);
    REQUIRE(close(tangent.x, 0.4472135954999579));
    REQUIRE(close(tangent.y, 0.8944271909999159));
    curve.getTangent(0.5, tangent);
    REQUIRE(close(tangent.x, 0.8944271909999159));
    REQUIRE(close(tangent.y, -0.4472135954999579));
    curve.getTangent(1, tangent);
    REQUIRE(close(tangent.x, 0.7071067811865475));
    REQUIRE(close(tangent.y, -0.7071067811865475));
}

TEST_CASE("NURBSCurve copy, clone and JSON keep the pinned fields", "[nurbs]") {
    NURBSCurve curve(2, knots, control);
    curve.arcLengthDivisions = 64;
    auto clone = curve.clone();
    auto typed = std::dynamic_pointer_cast<NURBSCurve>(clone);
    REQUIRE(typed != nullptr);
    REQUIRE(typed->degree == 2);
    REQUIRE(typed->knots == knots);
    REQUIRE(typed->controlPoints.size() == 5);
    REQUIRE(typed->arcLengthDivisions == 64);
    REQUIRE(typed->startKnot == 0);
    REQUIRE(typed->endKnot == 7);
    typed->controlPoints[0].x = 5.;
    REQUIRE(curve.controlPoints[0].x == 0.);
    const JsonValue json = curve.toJSON();
    REQUIRE(json.at("degree").get<double>() == 2.);
    REQUIRE(json.at("startKnot").get<double>() == 0.);
    REQUIRE(json.at("endKnot").get<double>() == 7.);
    REQUIRE(json.at("knots").get<JsonValue::Array>().size() == knots.size());
    const auto& points = json.at("controlPoints").get<JsonValue::Array>();
    REQUIRE(points.size() == 5);
    REQUIRE(points[1].get<JsonValue::Array>().size() == 4);
    REQUIRE(points[1].get<JsonValue::Array>()[1].get<double>() == 2.);
    NURBSCurve restored;
    restored.fromJSON(json);
    REQUIRE(restored.degree == 2.);
    REQUIRE(restored.knots == knots);
    REQUIRE(restored.controlPoints.size() == 5);
    REQUIRE(restored.controlPoints[1].y == 2.);
    REQUIRE(restored.controlPoints[4].w == 1.);
    REQUIRE(restored.startKnot == 0.);
    REQUIRE(restored.endKnot == 7.);
}

#include "catch.hpp"
#include "extras/curves/CubicBezierCurve.h"

namespace {
void expectedPoint(const Vector2& actual, std::initializer_list<double> expected) {
    const auto values = actual.toArray();
    REQUIRE(values.size() == expected.size());
    auto i = std::size_t{0};
    for (const auto value : expected) REQUIRE(values[i++] == Approx(value).margin(5e-12).epsilon(5e-12));
}
}

TEST_CASE("CubicBezierCurve matches pinned three.js points arc length and finite-difference tangents", "[extras][CubicBezierCurve]") {
    CubicBezierCurve curve(std::make_shared<Vector2>(-10, 0),
                     std::make_shared<Vector2>(-5, 15),
                     std::make_shared<Vector2>(20, 15),
                     std::make_shared<Vector2>(10, 0));
    REQUIRE(curve.isCubicBezierCurve);
    REQUIRE(curve.type == "CubicBezierCurve");
    const auto points = curve.getPoints(4);
    expectedPoint(*points[0], {-10, 0});
    expectedPoint(*points[1], {-3.359375, 8.4375});
    expectedPoint(*points[2], {5.625, 11.25});
    expectedPoint(*points[3], {11.796875, 8.4375});
    expectedPoint(*points[4], {10, 0});
    expectedPoint(curve.getPointAt(.3), {-3.3188282598022596, 8.463722639089221});
    expectedPoint(curve.getTangent(.25), {0.838961283088303, 0.5441911111721949});
    expectedPoint(curve.getTangentAt(.75), {0.5004110404199416, -0.8657879593906534});
    REQUIRE(curve.getLength() == Approx(36.64630888504102).epsilon(5e-12));
    Vector2 target;
    REQUIRE(&curve.getPoint(.5, target) == &target);
    CubicBezierCurve zero;
    REQUIRE(zero.getPoint(.5).equals(Vector2()));
    REQUIRE(zero.getTangent(.5).equals(Vector2()));
    REQUIRE(zero.getUtoTmapping(.3) == .5);
}

TEST_CASE("CubicBezierCurve controls share on construction and clone independently", "[extras][CubicBezierCurve]") {
    const auto shared = std::make_shared<Vector2>(1, 2);
    CubicBezierCurve source(shared, shared, shared, shared);
    REQUIRE(source.v0 == shared);
    shared->setScalar(7);
    REQUIRE(source.getPoint(.5).equals(*shared));
    source.arcLengthDivisions = 9;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<CubicBezierCurve>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v0 != source.v0);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->v3 != source.v3);
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 9);
    REQUIRE(clone->toJSON() == source.toJSON());
    CubicBezierCurve destination;
    const auto control = destination.v0;
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.v0 == control);
    REQUIRE(destination.toJSON() == source.toJSON());
    REQUIRE(&destination.fromJSON(source.toJSON()) == &destination);
    REQUIRE(destination.v0 == control);
}

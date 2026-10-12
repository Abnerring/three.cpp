#include "catch.hpp"
#include "extras/curves/QuadraticBezierCurve.h"

namespace {
void expectedPoint(const Vector2& actual, std::initializer_list<double> expected) {
    const auto values = actual.toArray();
    REQUIRE(values.size() == expected.size());
    auto i = std::size_t{0};
    for (const auto value : expected) REQUIRE(values[i++] == Approx(value).margin(5e-12).epsilon(5e-12));
}
}

TEST_CASE("QuadraticBezierCurve matches pinned three.js points arc length and finite-difference tangents", "[extras][QuadraticBezierCurve]") {
    QuadraticBezierCurve curve(std::make_shared<Vector2>(-10, 0),
                     std::make_shared<Vector2>(-5, 15),
                     std::make_shared<Vector2>(20, 15));
    REQUIRE(curve.isQuadraticBezierCurve);
    REQUIRE(curve.type == "QuadraticBezierCurve");
    const auto points = curve.getPoints(4);
    expectedPoint(*points[0], {-10, 0});
    expectedPoint(*points[1], {-6.25, 6.5625});
    expectedPoint(*points[2], {0, 11.25});
    expectedPoint(*points[3], {8.75, 14.0625});
    expectedPoint(*points[4], {20, 15});
    expectedPoint(curve.getPointAt(.3), {-3.9384722379170087, 8.752446654520154});
    expectedPoint(curve.getTangent(.25), {0.6643638388298648, 0.7474093186837085});
    expectedPoint(curve.getTangentAt(.75), {0.9905687912995924, 0.13701631181455934});
    REQUIRE(curve.getLength() == Approx(35.99678147334001).epsilon(5e-12));
    Vector2 target;
    REQUIRE(&curve.getPoint(.5, target) == &target);
    QuadraticBezierCurve zero;
    REQUIRE(zero.getPoint(.5).equals(Vector2()));
    REQUIRE(zero.getTangent(.5).equals(Vector2()));
    REQUIRE(zero.getUtoTmapping(.3) == .5);
}

TEST_CASE("QuadraticBezierCurve controls share on construction and clone independently", "[extras][QuadraticBezierCurve]") {
    const auto shared = std::make_shared<Vector2>(1, 2);
    QuadraticBezierCurve source(shared, shared, shared);
    REQUIRE(source.v0 == shared);
    shared->setScalar(7);
    REQUIRE(source.getPoint(.5).equals(*shared));
    source.arcLengthDivisions = 9;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<QuadraticBezierCurve>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v0 != source.v0);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 9);
    REQUIRE(clone->toJSON() == source.toJSON());
    QuadraticBezierCurve destination;
    const auto control = destination.v0;
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.v0 == control);
    REQUIRE(destination.toJSON() == source.toJSON());
    REQUIRE(&destination.fromJSON(source.toJSON()) == &destination);
    REQUIRE(destination.v0 == control);
}

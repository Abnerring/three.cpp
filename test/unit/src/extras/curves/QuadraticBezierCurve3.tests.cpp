#include "catch.hpp"
#include "extras/curves/QuadraticBezierCurve3.h"

namespace {
void expectedPoint(const Vector3& actual, std::initializer_list<double> expected) {
    const auto values = actual.toArray();
    REQUIRE(values.size() == expected.size());
    auto i = std::size_t{0};
    for (const auto value : expected) REQUIRE(values[i++] == Approx(value).margin(5e-12).epsilon(5e-12));
}
}

TEST_CASE("QuadraticBezierCurve3 matches pinned three.js points arc length and finite-difference tangents", "[extras][QuadraticBezierCurve3]") {
    QuadraticBezierCurve3 curve(std::make_shared<Vector3>(-10, 0, 1),
                     std::make_shared<Vector3>(-5, 15, -2),
                     std::make_shared<Vector3>(20, 15, 4));
    REQUIRE(curve.isQuadraticBezierCurve3);
    REQUIRE(curve.type == "QuadraticBezierCurve3");
    const auto points = curve.getPoints(4);
    expectedPoint(*points[0], {-10, 0, 1});
    expectedPoint(*points[1], {-6.25, 6.5625, 0.0625});
    expectedPoint(*points[2], {0, 11.25, 0.25});
    expectedPoint(*points[3], {8.75, 14.0625, 1.5625});
    expectedPoint(*points[4], {20, 15, 4});
    expectedPoint(curve.getPointAt(.3), {-3.888328610071, 8.792455647492647, 0.004913552285019329});
    expectedPoint(curve.getTangent(.25), {0.6635406438349054, 0.7464832243143791, -0.04976554828762251});
    expectedPoint(curve.getTangentAt(.75), {0.9716091626920977, 0.13230376524594048, 0.19614114477719322});
    REQUIRE(curve.getLength() == Approx(36.42657703061404).epsilon(5e-12));
    Vector3 target;
    REQUIRE(&curve.getPoint(.5, target) == &target);
    QuadraticBezierCurve3 zero;
    REQUIRE(zero.getPoint(.5).equals(Vector3()));
    REQUIRE(zero.getTangent(.5).equals(Vector3()));
    REQUIRE(zero.getUtoTmapping(.3) == .5);
}

TEST_CASE("QuadraticBezierCurve3 controls share on construction and clone independently", "[extras][QuadraticBezierCurve3]") {
    const auto shared = std::make_shared<Vector3>(1, 2, 3);
    QuadraticBezierCurve3 source(shared, shared, shared);
    REQUIRE(source.v0 == shared);
    shared->setScalar(7);
    REQUIRE(source.getPoint(.5).equals(*shared));
    source.arcLengthDivisions = 9;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<QuadraticBezierCurve3>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v0 != source.v0);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 9);
    REQUIRE(clone->toJSON() == source.toJSON());
    QuadraticBezierCurve3 destination;
    const auto control = destination.v0;
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.v0 == control);
    REQUIRE(destination.toJSON() == source.toJSON());
    REQUIRE(&destination.fromJSON(source.toJSON()) == &destination);
    REQUIRE(destination.v0 == control);
}

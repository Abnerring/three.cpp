#include "catch.hpp"
#include "extras/curves/CubicBezierCurve3.h"

namespace {
void expectedPoint(const Vector3& actual, std::initializer_list<double> expected) {
    const auto values = actual.toArray();
    REQUIRE(values.size() == expected.size());
    auto i = std::size_t{0};
    for (const auto value : expected) REQUIRE(values[i++] == Approx(value).margin(5e-12).epsilon(5e-12));
}
}

TEST_CASE("CubicBezierCurve3 matches pinned three.js points arc length and finite-difference tangents", "[extras][CubicBezierCurve3]") {
    CubicBezierCurve3 curve(std::make_shared<Vector3>(-10, 0, 1),
                     std::make_shared<Vector3>(-5, 15, -2),
                     std::make_shared<Vector3>(20, 15, 4),
                     std::make_shared<Vector3>(10, 0, 8));
    REQUIRE(curve.isCubicBezierCurve3);
    REQUIRE(curve.type == "CubicBezierCurve3");
    const auto points = curve.getPoints(4);
    expectedPoint(*points[0], {-10, 0, 1});
    expectedPoint(*points[1], {-3.359375, 8.4375, 0.265625});
    expectedPoint(*points[2], {5.625, 11.25, 1.875});
    expectedPoint(*points[3], {11.796875, 8.4375, 4.796875});
    expectedPoint(*points[4], {10, 0, 8});
    expectedPoint(curve.getPointAt(.3), {-3.0146781681684076, 8.655535074762598, 0.29154373873770556});
    expectedPoint(curve.getTangent(.25), {0.837507134736546, 0.5432478797939376, 0.05885185098849756});
    expectedPoint(curve.getTangentAt(.75), {0.40528289851785765, -0.7954634443263067, 0.4505371027002027});
    REQUIRE(curve.getLength() == Approx(38.02549499327321).epsilon(5e-12));
    Vector3 target;
    REQUIRE(&curve.getPoint(.5, target) == &target);
    CubicBezierCurve3 zero;
    REQUIRE(zero.getPoint(.5).equals(Vector3()));
    REQUIRE(zero.getTangent(.5).equals(Vector3()));
    REQUIRE(zero.getUtoTmapping(.3) == .5);
}

TEST_CASE("CubicBezierCurve3 controls share on construction and clone independently", "[extras][CubicBezierCurve3]") {
    const auto shared = std::make_shared<Vector3>(1, 2, 3);
    CubicBezierCurve3 source(shared, shared, shared, shared);
    REQUIRE(source.v0 == shared);
    shared->setScalar(7);
    REQUIRE(source.getPoint(.5).equals(*shared));
    source.arcLengthDivisions = 9;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<CubicBezierCurve3>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v0 != source.v0);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->v3 != source.v3);
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 9);
    REQUIRE(clone->toJSON() == source.toJSON());
    CubicBezierCurve3 destination;
    const auto control = destination.v0;
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.v0 == control);
    REQUIRE(destination.toJSON() == source.toJSON());
    REQUIRE(&destination.fromJSON(source.toJSON()) == &destination);
    REQUIRE(destination.v0 == control);
}

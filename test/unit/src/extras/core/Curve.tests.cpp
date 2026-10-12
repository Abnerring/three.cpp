#include "catch.hpp"
#include "extras/core/Curve.h"
#include "extras/curves/LineCurve3.h"

namespace {
class CountingCurve : public Curve<Vector2> {
  public:
    int calls = 0;
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override {
        ++calls;
        return target.set(t * t, 0);
    }
};
}

TEST_CASE("Curve base preserves defaults and serialization", "[extras][Curve]") {
    Curve<Vector2> curve;
    REQUIRE(curve.type == "Curve");
    REQUIRE(curve.arcLengthDivisions == 200);
    REQUIRE_FALSE(curve.needsUpdate);
    REQUIRE(curve.cacheArcLengths == nullptr);
    REQUIRE(curve.toJSON().at("metadata").at("version").get<double>() == 4.7);
    REQUIRE(curve.toJSON().at("metadata").at("generator").get<std::string>() == "Curve.toJSON");
    curve.arcLengthDivisions = 17;
    const auto cloned = curve.clone();
    REQUIRE(cloned.get() != &curve);
    REQUIRE(cloned->arcLengthDivisions == 17);
    Curve<Vector2> restored;
    REQUIRE(&restored.fromJSON(curve.toJSON()) == &restored);
    REQUIRE(restored.arcLengthDivisions == 17);
    REQUIRE_THROWS_AS(curve.getPoint(.5), std::logic_error);
}

TEST_CASE("Curve arc cache retains identity and only recomputes when required", "[extras][Curve]") {
    CountingCurve curve;
    curve.arcLengthDivisions = 4;
    const auto first = curve.getLengths();
    REQUIRE(*first == std::vector<double>{0, .0625, .25, .5625, 1});
    REQUIRE(curve.calls == 5);
    REQUIRE(curve.getLengths() == first);
    REQUIRE(curve.calls == 5);
    (*first)[2] = .125;
    REQUIRE(curve.getLengths()->at(2) == .125);
    curve.needsUpdate = true;
    const auto second = curve.getLengths();
    REQUIRE(second != first);
    REQUIRE(second->at(2) == .25);
    REQUIRE(first->at(2) == .125);
    REQUIRE_FALSE(curve.needsUpdate);
    REQUIRE(curve.calls == 10);
    curve.updateArcLengths();
    REQUIRE(curve.cacheArcLengths != second);
    REQUIRE_FALSE(curve.needsUpdate);
    REQUIRE(curve.calls == 15);
    REQUIRE(curve.getLength() == 1);
    const auto fractional = curve.getLengths(2.5);
    REQUIRE(fractional->size() == 3);
    REQUIRE(curve.getLengths(2.5) != fractional);
}

TEST_CASE("Curve distance mapping preserves truthiness and out-of-domain NaN", "[extras][Curve]") {
    CountingCurve curve;
    curve.arcLengthDivisions = 4;
    REQUIRE(curve.getUtoTmapping(.3) == Approx(.54));
    REQUIRE(curve.getUtoTmapping(.3, 0) == Approx(.54));
    REQUIRE(curve.getUtoTmapping(.3, -0.0) == Approx(.54));
    REQUIRE(curve.getUtoTmapping(.3, std::numeric_limits<double>::quiet_NaN()) == Approx(.54));
    REQUIRE(curve.getUtoTmapping(0, .5) == Approx(.7));
    REQUIRE(curve.getUtoTmapping(0) == 0);
    REQUIRE(curve.getUtoTmapping(1) == 1);
    REQUIRE(std::isnan(curve.getUtoTmapping(-1)));
    REQUIRE(std::isnan(curve.getUtoTmapping(2)));
    REQUIRE(std::isnan(curve.getUtoTmapping(.3, -1)));
    REQUIRE(std::isnan(curve.getUtoTmapping(std::numeric_limits<double>::infinity())));
    curve.arcLengthDivisions = 0;
    REQUIRE(*curve.getLengths() == std::vector<double>{0});
    REQUIRE(std::isnan(curve.getUtoTmapping(.3)));
}

TEST_CASE("Curve point and tangent helpers return the supplied vector", "[extras][Curve]") {
    CountingCurve curve;
    curve.arcLengthDivisions = 4;
    Vector2 target;
    REQUIRE(&curve.getPointAt(.3, target) == &target);
    REQUIRE(target.equals(Vector2(.2916, 0)));
    REQUIRE(&curve.getTangent(.5, target) == &target);
    REQUIRE(target.x == Approx(1));
    REQUIRE(target.y == 0);
    REQUIRE(&curve.getTangentAt(.5, target) == &target);
    REQUIRE(target.x == Approx(1));
    REQUIRE(target.y == 0);
    REQUIRE(curve.getPoints().size() == 6);
    REQUIRE(curve.getSpacedPoints().size() == 6);
    REQUIRE(curve.getPoints(-1).empty());
    REQUIRE(curve.getPoints(2.5).size() == 3);
    const auto zero = curve.getPoints(0);
    REQUIRE(zero.size() == 1);
    REQUIRE(std::isnan(zero[0]->x));
}

TEST_CASE("Curve copy does not reset existing cache or update state", "[extras][Curve]") {
    CountingCurve source, destination;
    source.arcLengthDivisions = 4;
    destination.arcLengthDivisions = 4;
    const auto cache = destination.getLengths();
    destination.type = "Custom";
    destination.needsUpdate = true;
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.cacheArcLengths == cache);
    REQUIRE(destination.needsUpdate);
    REQUIRE(destination.type == "Custom");
}

TEST_CASE("Curve Frenet frames follow upstream axis selection and closed correction", "[extras][Curve]") {
    LineCurve3 line(std::make_shared<Vector3>(), std::make_shared<Vector3>(10, 0, 0));
    for (const bool closed : {false, true}) {
        const auto frames = line.computeFrenetFrames(4, closed);
        REQUIRE(frames.tangents.size() == 5);
        REQUIRE(frames.normals.size() == 5);
        REQUIRE(frames.binormals.size() == 5);
        for (std::size_t i = 0; i < frames.tangents.size(); ++i) {
            REQUIRE(frames.tangents[i].equals(Vector3(1, 0, 0)));
            REQUIRE(frames.normals[i].equals(Vector3(0, 0, -1)));
            REQUIRE(frames.binormals[i].equals(Vector3(0, 1, 0)));
        }
    }
    LineCurve3 zero;
    const auto frames = zero.computeFrenetFrames(2);
    for (const auto& normal : frames.normals) REQUIRE(normal.lengthSq() == 0);
    REQUIRE(line.computeFrenetFrames(0).tangents.size() == 1);
}

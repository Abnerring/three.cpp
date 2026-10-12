#include "catch.hpp"
#include "extras/curves/LineCurve3.h"

TEST_CASE("LineCurve3 shares constructor controls and preserves analytic target behavior", "[extras][LineCurve3]") {
    const auto start = std::make_shared<Vector3>();
    const auto end = std::make_shared<Vector3>(3, 4, 12);
    LineCurve3 curve(start, end);
    REQUIRE(curve.isLineCurve3);
    REQUIRE(curve.type == "LineCurve3");
    REQUIRE(curve.v1 == start);
    REQUIRE(curve.v2 == end);
    REQUIRE(curve.getPoint(.5).equals(Vector3(1.5, 2, 6)));
    Vector3 target;
    REQUIRE(&curve.getPointAt(2, target) == &target);
    REQUIRE(target.equals(Vector3(6, 8, 24)));
    REQUIRE(&curve.getTangentAt(-1, target) == &target);
    REQUIRE(target.equals(Vector3(3.0 / 13, 4.0 / 13, 12.0 / 13)));
    REQUIRE(curve.getLength() == Approx(13));
    REQUIRE(curve.getPoints().size() == 6);
    end->setScalar(10);
    REQUIRE(curve.getPoint(1).equals(*end));
    REQUIRE(&curve.getPoint(.5, *start) == start.get());
    REQUIRE(start->equals(Vector3()));
    LineCurve3 zero;
    REQUIRE(zero.getTangent(.5).lengthSq() == 0);
    REQUIRE(zero.getUtoTmapping(.25) == .5);
}

TEST_CASE("LineCurve3 clone copy and JSON preserve upstream ownership", "[extras][LineCurve3]") {
    LineCurve3 source(std::make_shared<Vector3>(), std::make_shared<Vector3>(3, 4, 12));
    source.arcLengthDivisions = 4;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<LineCurve3>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->v2->equals(*source.v2));
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 4);
    LineCurve3 copy;
    const auto start = copy.v1, end = copy.v2;
    REQUIRE(&copy.copy(source) == &copy);
    REQUIRE(copy.v1 == start);
    REQUIRE(copy.v2 == end);
    REQUIRE(copy.toJSON() == source.toJSON());
    LineCurve3 restored;
    const auto restoredStart = restored.v1;
    REQUIRE(&restored.fromJSON(source.toJSON()) == &restored);
    REQUIRE(restored.v1 == restoredStart);
    REQUIRE(restored.toJSON() == source.toJSON());
}

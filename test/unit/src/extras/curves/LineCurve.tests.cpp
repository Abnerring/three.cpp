#include "catch.hpp"
#include "extras/curves/LineCurve.h"

TEST_CASE("LineCurve shares constructor controls and preserves analytic target behavior", "[extras][LineCurve]") {
    const auto start = std::make_shared<Vector2>();
    const auto end = std::make_shared<Vector2>(3, 4);
    LineCurve curve(start, end);
    REQUIRE(curve.isLineCurve);
    REQUIRE(curve.type == "LineCurve");
    REQUIRE(curve.v1 == start);
    REQUIRE(curve.v2 == end);
    REQUIRE(curve.getPoint(.5).equals(Vector2(1.5, 2)));
    Vector2 target;
    REQUIRE(&curve.getPointAt(2, target) == &target);
    REQUIRE(target.equals(Vector2(6, 8)));
    REQUIRE(&curve.getTangentAt(-1, target) == &target);
    REQUIRE(target.x == Approx(3.0 / 5));
    REQUIRE(target.y == Approx(4.0 / 5));
    REQUIRE(curve.getLength() == Approx(5));
    REQUIRE(curve.getPoints().size() == 6);
    end->setScalar(10);
    REQUIRE(curve.getPoint(1).equals(*end));
    REQUIRE(&curve.getPoint(.5, *start) == start.get());
    REQUIRE(start->equals(Vector2()));
    LineCurve zero;
    REQUIRE(zero.getTangent(.5).lengthSq() == 0);
    REQUIRE(zero.getUtoTmapping(.25) == .5);
}

TEST_CASE("LineCurve clone copy and JSON preserve upstream ownership", "[extras][LineCurve]") {
    LineCurve source(std::make_shared<Vector2>(), std::make_shared<Vector2>(3, 4));
    source.arcLengthDivisions = 4;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<LineCurve>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->v1 != source.v1);
    REQUIRE(clone->v2 != source.v2);
    REQUIRE(clone->v2->equals(*source.v2));
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 4);
    LineCurve copy;
    const auto start = copy.v1, end = copy.v2;
    REQUIRE(&copy.copy(source) == &copy);
    REQUIRE(copy.v1 == start);
    REQUIRE(copy.v2 == end);
    REQUIRE(copy.toJSON() == source.toJSON());
    LineCurve restored;
    const auto restoredStart = restored.v1;
    REQUIRE(&restored.fromJSON(source.toJSON()) == &restored);
    REQUIRE(restored.v1 == restoredStart);
    REQUIRE(restored.toJSON() == source.toJSON());
}

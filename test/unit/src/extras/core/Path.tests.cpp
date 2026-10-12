#include "catch.hpp"
#include "extras/core/Path.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/SplineCurve.h"
#include "extras/curves/EllipseCurve.h"

TEST_CASE("Path appends all curve types and preserves currentPoint identity", "[extras][Path]") {
    Path path;
    auto current = path.currentPoint;
    REQUIRE(&path.moveTo(1, 2) == &path);
    REQUIRE(&path.lineTo(3, 4) == &path);
    path.quadraticCurveTo(4, 8, 6, 4).bezierCurveTo(7, 1, 9, 8, 10, 4);
    Path::Points points{std::make_shared<Vector2>(11, 8), std::make_shared<Vector2>(12, 4)};
    path.splineThru(points);
    REQUIRE(path.currentPoint == current);
    REQUIRE(current->equals(Vector2(12, 4)));
    REQUIRE(path.curves->size() == 4);
    auto spline = std::dynamic_pointer_cast<SplineCurve>(path.curves->back());
    REQUIRE(spline->points->at(1) == points[0]);
    REQUIRE(spline->points->at(2) == points[1]);
    REQUIRE(spline->points->front() != current);
    points[1]->set(20, 20);
    REQUIRE(spline->getPoint(1).equals(*points[1]));
    REQUIRE(current->equals(Vector2(12, 4)));
    path.absarc(20, 10, 2, 0, std::numbers::pi);
    REQUIRE(path.curves->size() == 6); // connecting line, then ellipse
    REQUIRE(path.curves->at(4)->type == "LineCurve");
    REQUIRE(path.curves->at(5)->type == "EllipseCurve");
    REQUIRE(current->x == Approx(18));
    auto copy = std::dynamic_pointer_cast<Path>(path.clone());
    REQUIRE(copy->currentPoint != current);
    REQUIRE(copy->toJSON() == path.toJSON());
    Path restored;
    auto targetCurrent = restored.currentPoint;
    restored.fromJSON(path.toJSON());
    REQUIRE(restored.currentPoint == targetCurrent);
    REQUIRE(restored.toJSON() == path.toJSON());
}

TEST_CASE("Path setFromPoints appends and empty spline fails after insertion", "[extras][Path]") {
    auto input = std::make_shared<Path::Points>();
    input->push_back(std::make_shared<Vector2>(1, 2));
    input->push_back(std::make_shared<Vector2>(3, 4));
    Path path(input);
    input->at(0)->x = 99;
    auto line = std::dynamic_pointer_cast<LineCurve>(path.curves->front());
    REQUIRE(line->v1->x == 1);
    path.setFromPoints(input);
    REQUIRE(path.curves->size() == 2);
    REQUIRE_THROWS_AS(path.setFromPoints(Path::Points{}), std::out_of_range);
    REQUIRE_THROWS_AS(path.splineThru(Path::Points{}), std::out_of_range);
    REQUIRE(path.curves->size() == 3);
    REQUIRE(path.curves->back()->type == "SplineCurve");
    Path absolute, relative;
    absolute.absarc();
    relative.arc();
    REQUIRE(absolute.currentPoint->x == Approx(1));
    REQUIRE(std::isnan(relative.currentPoint->x));
    REQUIRE(std::isnan(relative.currentPoint->y));
}

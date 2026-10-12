#include "catch.hpp"
#include "extras/ShapeUtils.h"

TEST_CASE("ShapeUtils area and triangulation preserve endpoint mutation and index order", "[extras][ShapeUtils]") {
    std::vector<Vector2> contour{{0,0}, {10,0}, {10,10}, {0,10}, {0,0}};
    std::vector<std::vector<Vector2>> holes{{{2,2},{2,4},{4,4},{4,2},{2,2}}};
    REQUIRE(ShapeUtils::area(contour) == 100);
    REQUIRE_FALSE(ShapeUtils::isClockWise(contour));
    REQUIRE(ShapeUtils::isClockWise(holes.front()));
    const auto faces = ShapeUtils::triangulateShape(contour, holes);
    REQUIRE(contour.size() == 4);
    REQUIRE(holes.front().size() == 4);
    REQUIRE(faces.size() == 8);
    auto vertices = contour;
    vertices.insert(vertices.end(), holes[0].begin(), holes[0].end());
    double area = 0;
    for (const auto& face : faces) {
        std::vector<Vector2> triangle{vertices.at(face[0]), vertices.at(face[1]), vertices.at(face[2])};
        area += std::abs(ShapeUtils::area(triangle));
    }
    REQUIRE(area == 96);
    std::vector<Vector2> empty;
    std::vector<std::vector<Vector2>> noHoles;
    REQUIRE(ShapeUtils::area(empty) == 0);
    REQUIRE(ShapeUtils::triangulateShape(empty, noHoles).empty());
}

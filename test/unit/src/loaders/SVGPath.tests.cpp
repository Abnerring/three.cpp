#include "loaders/SVGLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("SVGLoader starts a path with a move") {
    SVGLoader::PathCursor cursor;
    // Two pairs: the first starts the path and the second continues it.
    const auto points = SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move,
                                                       {10, 20, 30, 40}, cursor);
    REQUIRE(points.size() == 2);
    REQUIRE(points[0].x == 10);
    REQUIRE(points[0].y == 20);
    REQUIRE(points[1].x == 30);
    REQUIRE(points[1].y == 40);
    // The cursor ends on the last point, and the path began at the first.
    REQUIRE(cursor.point.x == 30);
    REQUIRE(cursor.point.y == 40);
    REQUIRE(cursor.firstPoint.x == 10);
    REQUIRE(cursor.firstPoint.y == 20);
}

TEST_CASE("SVGLoader continues a path with a line") {
    SVGLoader::PathCursor cursor;
    // A move first, then a line.
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {1, 1}, cursor);
    const auto points = SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Line,
                                                       {5, 5, 9, 9}, cursor);
    REQUIRE(points.size() == 2);
    REQUIRE(points[0].x == 5);
    REQUIRE(points[1].y == 9);
    // The first point of the line replaced the path's first point, which is what the source
    // does: it is set whenever a command begins.
    REQUIRE(cursor.firstPoint.x == 5);
}

TEST_CASE("SVGLoader moves one axis at a time") {
    SVGLoader::PathCursor cursor;
    // Start at three, three.
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {3, 3}, cursor);
    // A horizontal step keeps the vertical place.
    const auto horizontal = SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Horizontal,
                                                           {5, 7}, cursor);
    REQUIRE(horizontal.size() == 2);
    REQUIRE(horizontal[0].x == 5);
    REQUIRE(horizontal[0].y == 3);
    REQUIRE(horizontal[1].x == 7);
    REQUIRE(horizontal[1].y == 3);
    // A vertical step keeps the horizontal place, which is seven after the last step.
    const auto vertical = SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Vertical,
                                                         {5, 7}, cursor);
    REQUIRE(vertical.size() == 2);
    REQUIRE(vertical[0].x == 7);
    REQUIRE(vertical[0].y == 5);
    REQUIRE(vertical[1].x == 7);
    REQUIRE(vertical[1].y == 7);
}

TEST_CASE("SVGLoader ignores a half pair") {
    SVGLoader::PathCursor cursor;
    // One number cannot make a point, so a move with one number visits nothing.
    REQUIRE(SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {1}, cursor).empty());
    // A second number completes it.
    REQUIRE(SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {1, 2}, cursor).size() == 1);
    // The odd one at the end of a longer run is dropped as well.
    REQUIRE(SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Line, {1, 2, 3}, cursor).size() == 1);
}

TEST_CASE("SVGLoader leaves the control point on the visited point") {
    SVGLoader::PathCursor cursor;
    const auto points = SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Line,
                                                       {2, 4}, cursor);
    REQUIRE(points.size() == 1);
    // The control point follows the point, which the smooth commands rely on.
    REQUIRE(cursor.control.x == 2);
    REQUIRE(cursor.control.y == 4);
}

TEST_CASE("SVGLoader reads nothing from nothing") {
    SVGLoader::PathCursor cursor;
    REQUIRE(SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {}, cursor).empty());
    REQUIRE(SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Horizontal, {}, cursor).empty());
    // The cursor is untouched.
    REQUIRE(cursor.point.x == 0);
    REQUIRE(cursor.point.y == 0);
}

#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader closes an empty path without effect") {
    SVGLoader::PathCursor cursor;
    // Nothing has been visited, so there is nothing to close.
    REQUIRE_FALSE(SVGLoader::applyCloseCommand(cursor));
    // The cursor did not move.
    REQUIRE(cursor.point.x == 0);
    REQUIRE(cursor.point.y == 0);
    REQUIRE_FALSE(cursor.isFirstPoint);
}

TEST_CASE("SVGLoader closes a path back to its first point") {
    SVGLoader::PathCursor cursor;
    // A move and a line leave the cursor on the line's end.
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {1, 2}, cursor);
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Line, {5, 6}, cursor);
    REQUIRE(cursor.point.x == 5);
    // Closing returns to the first point.
    REQUIRE(SVGLoader::applyCloseCommand(cursor));
    // The line command overwrote the first point, so the close returns to the line's own
    // first point rather than the move's.
    REQUIRE(cursor.point.x == 5);
    REQUIRE(cursor.point.y == 6);
    // And marks the next command as the start of a fresh subpath.
    REQUIRE(cursor.isFirstPoint);
}

TEST_CASE("SVGLoader closes to the first point of the last command") {
    // The first point is overwritten by each command, so the close returns to the latest one.
    SVGLoader::PathCursor cursor;
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {1, 1}, cursor);
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Line, {9, 9}, cursor);
    SVGLoader::applyCloseCommand(cursor);
    REQUIRE(cursor.point.x == 9);
    REQUIRE(cursor.point.y == 9);
}

TEST_CASE("SVGLoader closes after a curve") {
    SVGLoader::PathCursor cursor;
    SVGLoader::applyStraightCommand(SVGLoader::LineCommand::Move, {3, 4}, cursor);
    SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Quadratic, {1, 2, 7, 8}, cursor);
    REQUIRE(cursor.point.x == 7);
    REQUIRE(SVGLoader::applyCloseCommand(cursor));
    // A curve does not set the first point, so the close returns to the move's.
    // The line command overwrote the first point, so the close returns to the line's own
    // first point rather than the move's.
    REQUIRE(cursor.point.x == 3);
}

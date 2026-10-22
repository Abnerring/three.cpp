#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader reflects a coordinate") {
    // Twice the point less the control.
    REQUIRE(SVGLoader::reflect(5, 3) == 7);
    REQUIRE(SVGLoader::reflect(0, 0) == 0);
    // A control on the far side reflects back past the point.
    REQUIRE(SVGLoader::reflect(1, 5) == -3);
    // A negative point works the same way.
    REQUIRE(SVGLoader::reflect(-2, 0) == -4);
}

TEST_CASE("SVGLoader reads a cubic segment") {
    SVGLoader::PathCursor cursor;
    // Six numbers: two controls and the end.
    const auto segments = SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Cubic,
                                                      {1, 2, 3, 4, 5, 6}, cursor);
    REQUIRE(segments.size() == 1);
    REQUIRE(segments[0].cubic);
    REQUIRE(segments[0].control1.x == 1);
    REQUIRE(segments[0].control1.y == 2);
    REQUIRE(segments[0].control2.x == 3);
    REQUIRE(segments[0].control2.y == 4);
    REQUIRE(segments[0].end.x == 5);
    REQUIRE(segments[0].end.y == 6);
    // The second control is what a following smooth command reflects.
    REQUIRE(cursor.control.x == 3);
    REQUIRE(cursor.point.x == 5);
}

TEST_CASE("SVGLoader reads a smooth cubic segment") {
    SVGLoader::PathCursor cursor;
    // Start at ten, ten with the control at twelve, ten.
    cursor.point.x = 10;
    cursor.control.x = 12;
    // Four numbers: the second control and the end.
    const auto segments = SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::SmoothCubic,
                                                      {20, 8, 30, 8}, cursor);
    REQUIRE(segments.size() == 1);
    REQUIRE(segments[0].cubic);
    // The first control is the reflection of twelve through ten, which is eight.
    REQUIRE(segments[0].control1.x == 8);
    REQUIRE(segments[0].control2.x == 20);
    REQUIRE(segments[0].end.x == 30);
}

TEST_CASE("SVGLoader reads a quadratic segment") {
    SVGLoader::PathCursor cursor;
    const auto segments = SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Quadratic,
                                                      {1, 2, 3, 4}, cursor);
    REQUIRE(segments.size() == 1);
    REQUIRE_FALSE(segments[0].cubic);
    REQUIRE(segments[0].control1.x == 1);
    REQUIRE(segments[0].end.x == 3);
    REQUIRE(segments[0].end.y == 4);
    // The single control is the one kept.
    REQUIRE(cursor.control.x == 1);
}

TEST_CASE("SVGLoader reads a smooth quadratic segment") {
    SVGLoader::PathCursor cursor;
    cursor.point.x = 10;
    cursor.control.x = 12;
    // Two numbers: only the end point.
    const auto segments = SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::SmoothQuadratic,
                                                      {7, 8}, cursor);
    REQUIRE(segments.size() == 1);
    REQUIRE_FALSE(segments[0].cubic);
    REQUIRE(segments[0].control1.x == 8);
    REQUIRE(segments[0].end.x == 7);
    REQUIRE(segments[0].end.y == 8);
}

TEST_CASE("SVGLoader drops an incomplete segment") {
    SVGLoader::PathCursor cursor;
    // Three numbers cannot make a quadratic.
    REQUIRE(SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Quadratic, {1, 2, 3}, cursor).empty());
    // Five cannot make a cubic.
    REQUIRE(SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Cubic,
                                        {1, 2, 3, 4, 5}, cursor).empty());
    // One cannot make a smooth quadratic.
    REQUIRE(SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::SmoothQuadratic, {1}, cursor).empty());
}

TEST_CASE("SVGLoader reads several segments in one command") {
    SVGLoader::PathCursor cursor;
    // Twelve numbers make two cubics.
    const auto segments = SVGLoader::applyCurveCommand(SVGLoader::CurveCommand::Cubic,
                                                      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, cursor);
    REQUIRE(segments.size() == 2);
    REQUIRE(segments[0].end.x == 5);
    REQUIRE(segments[1].control1.x == 7);
    REQUIRE(segments[1].end.x == 11);
    // The cursor ends on the last one.
    REQUIRE(cursor.point.x == 11);
}
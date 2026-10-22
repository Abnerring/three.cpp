#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader reads a triangle with either winding") {
    // Counter clockwise keeps the order it was written in.
    LDrawLoader::LineParser ccw("1 2 3 4 5 6 7 8 9 10", 1);
    const auto forward = LDrawLoader::readTriangle(ccw, true);
    REQUIRE(forward[0][0] == 1.0);
    REQUIRE(forward[1][0] == 4.0);
    REQUIRE(forward[2][0] == 7.0);

    // Clockwise is the same triangle written the other way, so the first and last swap.
    LDrawLoader::LineParser cw("1 2 3 4 5 6 7 8 9 10", 1);
    const auto reversed = LDrawLoader::readTriangle(cw, false);
    REQUIRE(reversed[0][0] == 7.0);
    REQUIRE(reversed[1][0] == 4.0);
    REQUIRE(reversed[2][0] == 1.0);
}

TEST_CASE("LDrawLoader reads a quad with either winding") {
    // Four vertices of three numbers each.
    LDrawLoader::LineParser ccw("1 2 3 4 5 6 7 8 9 10 11 12 13", 1);
    const auto forward = LDrawLoader::readQuad(ccw, true);
    REQUIRE(forward[0][0] == 1.0);
    REQUIRE(forward[3][0] == 10.0);

    // Reversing a quad swaps both ends and the two in the middle.
    LDrawLoader::LineParser cw("1 2 3 4 5 6 7 8 9 10 11 12 13", 1);
    const auto reversed = LDrawLoader::readQuad(cw, false);
    REQUIRE(reversed[0][0] == 10.0);
    REQUIRE(reversed[1][0] == 7.0);
    REQUIRE(reversed[2][0] == 4.0);
    REQUIRE(reversed[3][0] == 1.0);
}

TEST_CASE("LDrawLoader decides whether a face shows from both sides") {
    // Certified and culling means one side only.
    REQUIRE_FALSE(LDrawLoader::isDoubleSided(true, true));
    // Never certified means both.
    REQUIRE(LDrawLoader::isDoubleSided(false, true));
    // Certified but not culling means both as well.
    REQUIRE(LDrawLoader::isDoubleSided(true, false));
    REQUIRE(LDrawLoader::isDoubleSided(false, false));
}

TEST_CASE("LDrawLoader counts a double sided triangle twice") {
    REQUIRE(LDrawLoader::triangleFaceCount(false) == 1);
    REQUIRE(LDrawLoader::triangleFaceCount(true) == 2);
    // The count follows the sides rather than being a constant.
    REQUIRE(LDrawLoader::triangleFaceCount(LDrawLoader::isDoubleSided(true, true)) == 1);
    REQUIRE(LDrawLoader::triangleFaceCount(LDrawLoader::isDoubleSided(false, true)) == 2);
}

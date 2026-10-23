#include "nodes/core/SpriteSheetUV.h"

#include "catch.hpp"

TEST_CASE("SpriteSheetUV counts a frame down to a whole number in the grid") {
    // A whole frame is itself.
    REQUIRE(SpriteSheetUV::frameNumber(0.0, 6, 6) == 0);
    REQUIRE(SpriteSheetUV::frameNumber(5.0, 6, 6) == 5);
    // A frame with a fraction is cut down rather than rounded.
    REQUIRE(SpriteSheetUV::frameNumber(5.9, 6, 6) == 5);
    // A frame past the end wraps round the whole grid.
    REQUIRE(SpriteSheetUV::frameNumber(36.0, 6, 6) == 0);
    REQUIRE(SpriteSheetUV::frameNumber(37.0, 6, 6) == 1);
    // The wrap is against the whole grid and not one row.
    REQUIRE(SpriteSheetUV::frameNumber(12.0, 6, 6) == 12);
}

TEST_CASE("SpriteSheetUV's column is the remainder against the width") {
    REQUIRE(SpriteSheetUV::column(0, 6) == 0);
    REQUIRE(SpriteSheetUV::column(5, 6) == 5);
    REQUIRE(SpriteSheetUV::column(6, 6) == 0);
    REQUIRE(SpriteSheetUV::column(7, 6) == 1);
    REQUIRE(SpriteSheetUV::column(11, 6) == 5);
}

TEST_CASE("SpriteSheetUV's row counts down from the top") {
    // The first frame is on the top row, which in a texture counted from the bottom is the last
    // row, not the first.
    REQUIRE(SpriteSheetUV::row(0, 6, 6) == 5);
    REQUIRE(SpriteSheetUV::row(5, 6, 6) == 5);
    // The next frame starts the row below, which is one less.
    REQUIRE(SpriteSheetUV::row(6, 6, 6) == 4);
    REQUIRE(SpriteSheetUV::row(11, 6, 6) == 4);
    // And the last row of the grid is reached only at the end.
    REQUIRE(SpriteSheetUV::row(30, 6, 6) == 0);
}

TEST_CASE("SpriteSheetUV's two forms for the row agree everywhere") {
    // The longer form with the ceiling and the one added is the same as the shorter one, which
    // a sweep shows rather than a remark; this is the property the longer form rests on.
    for (int frame = 0; frame < 36; ++frame) {
        REQUIRE(SpriteSheetUV::row(frame, 6, 6) == SpriteSheetUV::rowWouldBe(frame, 6, 6));
    }
    // And for a width that does not divide the count evenly, where a mistake would show.
    for (int frame = 0; frame < 25; ++frame) {
        REQUIRE(SpriteSheetUV::row(frame, 5, 5) == SpriteSheetUV::rowWouldBe(frame, 5, 5));
    }
    // The two are written differently, so at least one frame has a different intermediate value
    // even though the answers agree; the sweep above is what makes them safe to swap.
    REQUIRE(SpriteSheetUV::row(0, 6, 6) == SpriteSheetUV::rowWouldBe(0, 6, 6));
}

TEST_CASE("SpriteSheetUV's scale is one over the count") {
    REQUIRE(SpriteSheetUV::scale(6) == Approx(1.0 / 6.0).epsilon(1e-15));
    REQUIRE(SpriteSheetUV::scale(1) == 1.0);
    REQUIRE(SpriteSheetUV::scale(4) == 0.25);
    // Six cells of a sixth each add up to the whole, which is what a scale should do.
    REQUIRE(SpriteSheetUV::scale(6) * 6 == Approx(1.0).epsilon(1e-15));
}
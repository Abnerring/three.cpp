#include "nodes/core/RotateNode.h"

#include "catch.hpp"

#include <cmath>
#include <string>

using Pair = RotateNode::Pair;

TEST_CASE("RotateNode turns a two dimensional point anti clockwise") {
    const double half = 3.14159265358979323846 / 2.0;
    // A point on the first axis turns onto the second, which is anti clockwise.
    const Pair right{1.0, 0.0};
    const Pair up = RotateNode::rotate2D(right, half);
    REQUIRE(up.x == Approx(0.0).margin(1e-12));
    REQUIRE(up.y == Approx(1.0).epsilon(1e-12));
    // And a point on the second axis turns onto the first, negatively.
    const Pair back = RotateNode::rotate2D(Pair{0.0, 1.0}, half);
    REQUIRE(back.x == Approx(-1.0).epsilon(1e-12));
    REQUIRE(back.y == Approx(0.0).margin(1e-12));
    // The origin does not move.
    const Pair origin = RotateNode::rotate2D(Pair{0.0, 0.0}, 1.234);
    REQUIRE(origin.x == 0.0);
    REQUIRE(origin.y == 0.0);
}

TEST_CASE("RotateNode keeps a point's distance from the origin") {
    // A turn does not change how far a point is from the origin, which a sweep shows.
    for (int i = 0; i <= 12; ++i) {
        const double angle = i * 0.5235987755982988;
        const Pair out = RotateNode::rotate2D(Pair{0.6, 0.8}, angle);
        const double before = 0.6 * 0.6 + 0.8 * 0.8;
        const double after = out.x * out.x + out.y * out.y;
        REQUIRE(after == Approx(before).margin(1e-12));
    }
    // A full turn brings the point back.
    const Pair full = RotateNode::rotate2D(Pair{0.6, 0.8}, 6.283185307179586);
    REQUIRE(full.x == Approx(0.6).margin(1e-12));
    REQUIRE(full.y == Approx(0.8).margin(1e-12));
}

TEST_CASE("RotateNode's second axis is the one with the other sign") {
    // The first and third axes use one order for the sine and the second the other, so a test
    // that treated all three alike would get the second one wrong.
    REQUIRE(RotateNode::usesSharedSineOrder('X'));
    REQUIRE_FALSE(RotateNode::usesSharedSineOrder('Y'));
    REQUIRE(RotateNode::usesSharedSineOrder('Z'));
    // The first column of the second axis carries the negated sine where the third carries the
    // positive one, which is the difference itself.
    const double half = 3.14159265358979323846 / 2.0;
    const auto aboutY = RotateNode::firstColumn('Y', half);
    const auto aboutZ = RotateNode::firstColumn('Z', half);
    REQUIRE(aboutY.c == Approx(-1.0).epsilon(1e-12));
    REQUIRE(aboutZ.b == Approx(1.0).epsilon(1e-12));
    // The two are therefore not the same column, which is what the sign change means.
    REQUIRE(aboutY.c != aboutZ.b);
}

TEST_CASE("RotateNode multiplies its three matrices left to right") {
    // The first character of the order names the leftmost factor; the order therefore matters
    // and two different orders give different products.
    REQUIRE(RotateNode::orderIsLeftToRight());
    // Because of that, the order has to be part of what tells two nodes apart for caching.
    REQUIRE(RotateNode::cacheKeyIncludesOrder());
    // The default order's first axis is the first of the three.
    REQUIRE(RotateNode::usesSharedSineOrder('X'));
    REQUIRE(std::string(RotateNode::kTypeName) == "RotateNode");
}
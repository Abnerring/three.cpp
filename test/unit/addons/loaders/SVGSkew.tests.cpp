#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader reads a skew") {
    // A forty five degree skew leans by one, since its tangent is one.
    const auto sideways = SVGLoader::skewXMatrix({45});
    REQUIRE(sideways.c == Approx(1.0).epsilon(1e-12));
    // A horizontal skew leaves the height alone, so a point at one up moves one sideways.
    const auto moved = SVGLoader::transformPoint(sideways, {0, 1});
    REQUIRE(moved.x == Approx(1.0).epsilon(1e-12));
    REQUIRE(moved.y == Approx(1.0).epsilon(1e-12));

    // The vertical skew is the other way round.
    const auto upward = SVGLoader::skewYMatrix({45});
    REQUIRE(upward.b == Approx(1.0).epsilon(1e-12));
    const auto lifted = SVGLoader::transformPoint(upward, {1, 0});
    REQUIRE(lifted.x == Approx(1.0).epsilon(1e-12));
    REQUIRE(lifted.y == Approx(1.0).epsilon(1e-12));

    // No angle leaves the identity.
    REQUIRE(SVGLoader::skewXMatrix({}).c == 0);
    REQUIRE(SVGLoader::skewYMatrix({}).b == 0);
    // A zero angle is also the identity.
    REQUIRE(SVGLoader::skewXMatrix({0}).c == Approx(0.0).margin(1e-12));
}

TEST_CASE("SVGLoader composes transforms") {
    const SVGLoader::Matrix identity;
    const auto moved = SVGLoader::translateMatrix({5, 7});
    // The identity leaves a transform as it is on either side.
    const auto left = SVGLoader::multiply(identity, moved);
    REQUIRE(left.e == 5);
    REQUIRE(left.f == 7);
    const auto right = SVGLoader::multiply(moved, identity);
    REQUIRE(right.e == 5);
    REQUIRE(right.f == 7);
}

TEST_CASE("SVGLoader applies the right hand transform first") {
    // A move right then a move down adds up, in either order.
    const auto across = SVGLoader::translateMatrix({5, 0});
    const auto down = SVGLoader::translateMatrix({0, 7});
    const auto both = SVGLoader::multiply(across, down);
    REQUIRE(both.e == 5);
    REQUIRE(both.f == 7);
    // The same point arrives at the same place either way for two moves.
    const auto point = SVGLoader::transformPoint(both, {0, 0});
    REQUIRE(point.x == 5);
    REQUIRE(point.y == 7);
}

TEST_CASE("SVGLoader composes a scale with a move") {
    // Scaling after a move stretches the move as well.
    const auto move = SVGLoader::translateMatrix({2, 3});
    const auto grow = SVGLoader::scaleMatrix({10});
    const auto composed = SVGLoader::multiply(grow, move);
    // The move is multiplied by the scale because the right hand one acts first.
    REQUIRE(composed.e == 20);
    REQUIRE(composed.f == 30);
    // The other order leaves the move alone.
    const auto other = SVGLoader::multiply(move, grow);
    REQUIRE(other.e == 2);
    REQUIRE(other.f == 3);
    REQUIRE(other.a == 10);
}
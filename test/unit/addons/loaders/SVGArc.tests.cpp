#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader groups the numbers of an arc") {
    // Two radii, a rotation, two flags and an end point.
    const auto arcs = SVGLoader::parseArcSegments({1, 2, 30, 1, 0, 5, 6});
    REQUIRE(arcs.size() == 1);
    REQUIRE(arcs[0].rx == 1);
    REQUIRE(arcs[0].ry == 2);
    REQUIRE(arcs[0].rotation == 30);
    REQUIRE(arcs[0].largeArc);
    REQUIRE_FALSE(arcs[0].sweep);
    REQUIRE(arcs[0].end.x == 5);
    REQUIRE(arcs[0].end.y == 6);
}

TEST_CASE("SVGLoader reads several arcs from one command") {
    // fourteen numbers make two arcs.
    const auto arcs = SVGLoader::parseArcSegments(
        {1, 1, 0, 0, 0, 2, 2, 3, 3, 0, 0, 1, 4, 4});
    REQUIRE(arcs.size() == 2);
    REQUIRE(arcs[0].end.x == 2);
    REQUIRE(arcs[1].rx == 3);
    REQUIRE(arcs[1].sweep);
    REQUIRE(arcs[1].end.y == 4);
}

TEST_CASE("SVGLoader leaves an incomplete arc unread") {
    // Six numbers cannot fill seven.
    REQUIRE(SVGLoader::parseArcSegments({1, 2, 3, 4, 5, 6}).empty());
    // Eight numbers fill only the first arc; the eighth is left over.
    const auto arcs = SVGLoader::parseArcSegments({1, 1, 0, 0, 0, 2, 2, 9});
    REQUIRE(arcs.size() == 1);
    REQUIRE(arcs[0].end.x == 2);
}

TEST_CASE("SVGLoader treats any nonzero flag as set") {
    // Two is as much a flag as one.
    const auto arcs = SVGLoader::parseArcSegments({1, 1, 0, 2, 3, 0, 0});
    REQUIRE(arcs[0].largeArc);
    REQUIRE(arcs[0].sweep);
    // A negative value counts too.
    const auto negative = SVGLoader::parseArcSegments({1, 1, 0, -1, -1, 0, 0});
    REQUIRE(negative[0].largeArc);
    REQUIRE(negative[0].sweep);
}

TEST_CASE("SVGLoader skips an arc that goes nowhere") {
    // The same point twice has nothing to draw.
    REQUIRE(SVGLoader::shouldSkipArc({1, 2}, {1, 2}));
    // A single axis off is enough to draw.
    REQUIRE_FALSE(SVGLoader::shouldSkipArc({1, 2}, {1, 3}));
    REQUIRE_FALSE(SVGLoader::shouldSkipArc({1, 2}, {2, 2}));
    // The comparison is exact, so the smallest difference still draws.
    REQUIRE_FALSE(SVGLoader::shouldSkipArc({1, 2}, {1 + 1e-12, 2}));
}
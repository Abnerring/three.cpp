#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader moves a point by a transform") {
    // The identity leaves a point where it is.
    const SVGLoader::Matrix identity;
    const SVGLoader::Point point{1, 2};
    const auto same = SVGLoader::transformPoint(identity, point);
    REQUIRE(same.x == 1);
    REQUIRE(same.y == 2);
    // A translation moves it.
    SVGLoader::Matrix moved;
    moved.e = 5;
    moved.f = 6;
    const auto there = SVGLoader::transformPoint(moved, point);
    REQUIRE(there.x == 6);
    REQUIRE(there.y == 8);
    // A scale stretches it.
    SVGLoader::Matrix scaled;
    scaled.a = 2;
    scaled.d = 3;
    const auto big = SVGLoader::transformPoint(scaled, point);
    REQUIRE(big.x == 2);
    REQUIRE(big.y == 6);
}

TEST_CASE("SVGLoader reads a translate") {
    // One number moves only the horizontal axis.
    const auto one = SVGLoader::translateMatrix({3});
    REQUIRE(one.e == 3);
    REQUIRE(one.f == 0);
    // Two move both.
    const auto two = SVGLoader::translateMatrix({3, 4});
    REQUIRE(two.e == 3);
    REQUIRE(two.f == 4);
    // Nothing to read leaves the identity.
    const auto none = SVGLoader::translateMatrix({});
    REQUIRE(none.e == 0);
    REQUIRE(none.f == 0);
}

TEST_CASE("SVGLoader reads a scale") {
    // One number scales both axes alike.
    const auto one = SVGLoader::scaleMatrix({2});
    REQUIRE(one.a == 2);
    REQUIRE(one.d == 2);
    // Two scale them apart.
    const auto two = SVGLoader::scaleMatrix({2, 3});
    REQUIRE(two.a == 2);
    REQUIRE(two.d == 3);
    // Nothing leaves the identity.
    const auto none = SVGLoader::scaleMatrix({});
    REQUIRE(none.a == 1);
    REQUIRE(none.d == 1);
}

TEST_CASE("SVGLoader reads a rotate") {
    // A quarter turn puts the axes on each other.
    const auto quarter = SVGLoader::rotateMatrix({90});
    REQUIRE(quarter.a == Approx(0.0).margin(1e-12));
    REQUIRE(quarter.b == Approx(1.0).epsilon(1e-12));
    REQUIRE(quarter.c == Approx(-1.0).epsilon(1e-12));
    REQUIRE(quarter.d == Approx(0.0).margin(1e-12));
    // No turn leaves the identity.
    const auto none = SVGLoader::rotateMatrix({0});
    REQUIRE(none.a == Approx(1.0).epsilon(1e-12));
    REQUIRE(none.b == Approx(0.0).margin(1e-12));
    // Nothing to read leaves the identity too.
    REQUIRE(SVGLoader::rotateMatrix({}).a == 1);
}

TEST_CASE("SVGLoader turns about a centre") {
    // A half turn about five, five leaves the centre where it is.
    const auto about = SVGLoader::rotateMatrix({180, 5, 5});
    const SVGLoader::Point centre{5, 5};
    const auto fixed = SVGLoader::transformPoint(about, centre);
    REQUIRE(fixed.x == Approx(5.0).margin(1e-9));
    REQUIRE(fixed.y == Approx(5.0).margin(1e-9));
    // A point away from the centre lands on the other side of it.
    const SVGLoader::Point offset{7, 5};
    const auto opposite = SVGLoader::transformPoint(about, offset);
    REQUIRE(opposite.x == Approx(3.0).margin(1e-9));
    REQUIRE(opposite.y == Approx(5.0).margin(1e-9));
}
#include "math/Box2.h"
#include "math/Cylindrical.h"
#include "math/Line3.h"
#include "math/Spherical.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

#include "catch.hpp"

// Expected values come from running the pinned Line3.js, Box2.js, Cylindrical.js and
// Spherical.js in node.
TEST_CASE("Line3 center, delta, length and closest point match upstream") {
    Line3 line(Vector3(0, 0, 0), Vector3(2, 2, 2));
    Vector3 center, delta;
    line.getCenter(center);
    line.delta(delta);
    REQUIRE(center.x == 1);
    REQUIRE(center.y == 1);
    REQUIRE(center.z == 1);
    REQUIRE(delta.x == 2);
    REQUIRE(delta.y == 2);
    REQUIRE(delta.z == 2);
    REQUIRE(line.distance() == 3.4641016151377544);

    // Clamped to the segment: the projection is pulled back to the far end.
    Vector3 clamped;
    line.closestPointToPoint(Vector3(2, 0, 0), true, clamped);
    REQUIRE(clamped.x == 0.6666666666666666);
    REQUIRE(clamped.y == 0.6666666666666666);
    REQUIRE(clamped.z == 0.6666666666666666);

    // Unclamped: the projection may run past the end of the segment.
    Vector3 unclamped;
    line.closestPointToPoint(Vector3(0, 5, 0), false, unclamped);
    REQUIRE(unclamped.x == 1.6666666666666667);
    REQUIRE(unclamped.y == 1.6666666666666667);
    REQUIRE(unclamped.z == 1.6666666666666667);
}

TEST_CASE("Box2 center, size, containment, clamp and distance match upstream") {
    Box2 box(Vector2(-1, -2), Vector2(3, 4));
    Vector2 center, size;
    box.getCenter(center);
    box.getSize(size);
    REQUIRE(center.x == 1);
    REQUIRE(center.y == 1);
    REQUIRE(size.x == 4);
    REQUIRE(size.y == 6);

    REQUIRE(box.containsPoint(Vector2(0, 0)));
    REQUIRE_FALSE(box.containsPoint(Vector2(9, 9)));

    Vector2 clamped;
    box.clampPoint(Vector2(9, 9), clamped);
    REQUIRE(clamped.x == 3);
    REQUIRE(clamped.y == 4);
    REQUIRE(box.distanceToPoint(Vector2(9, 9)) == 7.810249675906654);
}

TEST_CASE("Cylindrical and Spherical conversions match upstream") {
    Cylindrical cylinder;
    cylinder.setFromVector3(Vector3(0, 3, 5));
    REQUIRE(cylinder.radius == 5);
    REQUIRE(cylinder.theta == 0);
    REQUIRE(cylinder.y == 3);

    Spherical spherical;
    spherical.setFromVector3(Vector3(0, 0, 5));
    REQUIRE(spherical.radius == 5);
    REQUIRE(spherical.phi == 1.5707963267948966);
    REQUIRE(spherical.theta == 0);

    // A point on the pole has no theta, and phi collapses to zero.
    Spherical pole;
    pole.setFromVector3(Vector3(0, 5, 0));
    REQUIRE(pole.radius == 5);
    REQUIRE(pole.phi == 0);
}

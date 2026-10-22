#include "math/Box3.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include "math/Triangle.h"
#include "math/Vector3.h"

#include "catch.hpp"

// All expected values come from running the pinned Sphere.js, Plane.js and Triangle.js in
// node, so they compare against upstream rather than against hand made numbers.
TEST_CASE("Sphere containment and distance follow the upstream sign convention") {
    Sphere sphere(Vector3(1, 2, 3), 5);
    // A point inside the sphere reports a negative distance to its surface.
    REQUIRE(sphere.containsPoint(Vector3(1, 2, 7)));
    REQUIRE(sphere.distanceToPoint(Vector3(1, 2, 7)) == -1.0);

    Box3 box(Vector3(0, 0, 0), Vector3(4, 4, 4));
    REQUIRE(sphere.intersectsBox(box));

    Plane plane(Vector3(0, 1, 0), -2);
    REQUIRE(sphere.intersectsPlane(plane));
}

TEST_CASE("Plane stores its normal and constant and projects points") {
    Plane plane(Vector3(0, 1, 0), -2);
    REQUIRE(plane.normal.x == 0);
    REQUIRE(plane.normal.y == 1);
    REQUIRE(plane.normal.z == 0);
    REQUIRE(plane.constant == -2);
    REQUIRE(plane.distanceToPoint(Vector3(0, 5, 0)) == 3);

    Vector3 projected;
    plane.projectPoint(Vector3(3, 5, 4), projected);
    REQUIRE(projected.x == 3);
    REQUIRE(projected.y == 2);
    REQUIRE(projected.z == 4);
}

TEST_CASE("Triangle reports area, barycentric coordinates and containment") {
    Triangle triangle;
    // A default triangle is degenerate and has no area.
    REQUIRE(triangle.getArea() == 0.0);

    triangle.set(Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0));
    REQUIRE(triangle.getArea() == 0.5);

    Vector3 barycoord;
    triangle.getBarycoord(Vector3(0.25, 0.25, 0), barycoord);
    REQUIRE(barycoord.x == 0.5);
    REQUIRE(barycoord.y == 0.25);
    REQUIRE(barycoord.z == 0.25);

    REQUIRE(triangle.containsPoint(Vector3(0.25, 0.25, 0)));
    REQUIRE_FALSE(triangle.containsPoint(Vector3(2, 2, 0)));

    Vector3 normal;
    triangle.getNormal(normal);
    REQUIRE(normal.x == 0);
    REQUIRE(normal.y == 0);
    REQUIRE(normal.z == 1);

    Vector3 midpoint;
    triangle.getMidpoint(midpoint);
    REQUIRE(midpoint.x == 1.0 / 3.0);
    REQUIRE(midpoint.y == 1.0 / 3.0);
    REQUIRE(midpoint.z == 0);
}

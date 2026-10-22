#include "math/Box3.h"
#include "math/Frustum.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Ray.h"
#include "math/Sphere.h"
#include "math/Triangle.h"
#include "math/Vector3.h"

#include "catch.hpp"

// Expected values come from running the pinned Ray.js and Frustum.js in node.
TEST_CASE("Ray at, distance and closest point match upstream") {
    Ray ray(Vector3(0, 0, 10), Vector3(0, 0, -1));
    Vector3 point;
    ray.at(4, point);
    REQUIRE(point.x == 0);
    REQUIRE(point.y == 0);
    REQUIRE(point.z == 6);

    REQUIRE(ray.distanceToPoint(Vector3(5, 0, 10)) == 5.0);
    REQUIRE(ray.distanceSqToPoint(Vector3(0, 3, 10)) == 9.0);

    // A query beyond the origin clamps to the origin, not to the projected point.
    Vector3 closest;
    ray.closestPointToPoint(Vector3(0, 0, 100), closest);
    REQUIRE(closest.x == 0);
    REQUIRE(closest.y == 0);
    REQUIRE(closest.z == 10);
}

TEST_CASE("Ray intersections report the near hit like upstream") {
    Ray ray(Vector3(0, 0, 10), Vector3(0, 0, -1));

    Vector3 sphereHit;
    REQUIRE(ray.intersectSphere(Sphere(Vector3(0, 0, 0), 2), sphereHit) != nullptr);
    REQUIRE(sphereHit.z == 2);

    Vector3 boxHit;
    REQUIRE(ray.intersectBox(Box3(Vector3(-1, -1, -1), Vector3(1, 1, 1)), boxHit) != nullptr);
    REQUIRE(boxHit.z == 1);

    Vector3 planeHit;
    REQUIRE(ray.intersectPlane(Plane(Vector3(0, 0, 1), 0), planeHit) != nullptr);
    REQUIRE(planeHit.z == 0);

    Vector3 triangleHit;
    Triangle triangle(Vector3(-1, -1, 0), Vector3(1, -1, 0), Vector3(0, 1, 0));
    REQUIRE(ray.intersectTriangle(triangle.a, triangle.b, triangle.c, false, triangleHit) != nullptr);
    REQUIRE(triangleHit.z == 0);
}

TEST_CASE("Ray distanceToPlane reports no distance for a parallel ray") {
    // Upstream returns null for a parallel ray, which is an empty optional here.
    Ray parallel(Vector3(0, 0, 0), Vector3(0, 0, -1));
    REQUIRE_FALSE(parallel.distanceToPlane(Plane(Vector3(0, 0, 1), -5)).has_value());
}

TEST_CASE("Frustum containment and intersection match upstream") {
    Frustum empty;
    // An empty frustum has zero planes, so nothing is excluded.
    REQUIRE(empty.containsPoint(Vector3(0, 0, 0)));

    Frustum frustum;
    Matrix4 projection;
    projection.makePerspective(-1, 1, 1, -1, 0.1, 100);
    frustum.setFromProjectionMatrix(projection, WebGLCoordinateSystem);

    REQUIRE(frustum.containsPoint(Vector3(0, 0, -5)));
    REQUIRE_FALSE(frustum.containsPoint(Vector3(0, 0, -500)));
    REQUIRE(frustum.intersectsBox(Box3(Vector3(-1, -1, -5), Vector3(1, 1, -4))));
    REQUIRE(frustum.intersectsSphere(Sphere(Vector3(0, 0, -5), 1)));
}

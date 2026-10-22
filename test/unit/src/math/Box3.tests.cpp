#include "math/Box3.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"

#include "catch.hpp"

// Expected values were produced by running the pinned Box3.js in node, so these compare
// against the upstream implementation rather than against hand rounded numbers.
TEST_CASE("Box3 starts empty and setFromPoints spans the points") {
    Box3 box;
    REQUIRE(box.isEmpty());

    box.setFromPoints({Vector3(1, 2, 3), Vector3(-1, 0, 5), Vector3(2, -3, 1)});
    REQUIRE_FALSE(box.isEmpty());
    REQUIRE(box.min.x == -1);
    REQUIRE(box.min.y == -3);
    REQUIRE(box.min.z == 1);
    REQUIRE(box.max.x == 2);
    REQUIRE(box.max.y == 2);
    REQUIRE(box.max.z == 5);
}

TEST_CASE("Box3 center, size, clamp, distance and parameter match upstream") {
    Box3 box;
    box.setFromPoints({Vector3(1, 2, 3), Vector3(-1, 0, 5), Vector3(2, -3, 1)});

    Vector3 center, size;
    box.getCenter(center);
    box.getSize(size);
    REQUIRE(center.x == 0.5);
    REQUIRE(center.y == -0.5);
    REQUIRE(center.z == 3);
    REQUIRE(size.x == 3);
    REQUIRE(size.y == 5);
    REQUIRE(size.z == 4);

    Vector3 clamped;
    box.clampPoint(Vector3(10, 10, 10), clamped);
    REQUIRE(clamped.x == 2);
    REQUIRE(clamped.y == 2);
    REQUIRE(clamped.z == 5);
    REQUIRE(box.distanceToPoint(Vector3(10, 10, 10)) == 12.36931687685298);

    Vector3 parameter;
    box.getParameter(Vector3(0, 0, 4), parameter);
    REQUIRE(parameter.x == 1.0 / 3.0);
    REQUIRE(parameter.y == 0.6);
    REQUIRE(parameter.z == 0.75);
}

TEST_CASE("Box3 containment follows the closed interval semantics") {
    Box3 box;
    box.setFromPoints({Vector3(1, 2, 3), Vector3(-1, 0, 5), Vector3(2, -3, 1)});
    REQUIRE(box.containsPoint(Vector3(0, 0, 4)));
    REQUIRE_FALSE(box.containsPoint(Vector3(9, 9, 9)));
    // A point exactly on the boundary counts as contained.
    REQUIRE(box.containsPoint(Vector3(-1, -3, 1)));
}

TEST_CASE("Box3 union and intersect match upstream") {
    Box3 box(Vector3(-1, -3, 1), Vector3(2, 2, 5));
    Box3 other(Vector3(0, 0, 0), Vector3(4, 4, 4));

    Box3 united = box.clone();
    united.unionWith(other);
    REQUIRE(united.min.x == -1);
    REQUIRE(united.min.y == -3);
    REQUIRE(united.min.z == 0);
    REQUIRE(united.max.x == 4);
    REQUIRE(united.max.y == 4);
    REQUIRE(united.max.z == 5);

    Box3 clipped = box.clone();
    clipped.intersect(other);
    REQUIRE_FALSE(clipped.isEmpty());
    REQUIRE(clipped.min.x == 0);
    REQUIRE(clipped.min.y == 0);
    REQUIRE(clipped.min.z == 1);
    REQUIRE(clipped.max.x == 2);
    REQUIRE(clipped.max.y == 2);
    REQUIRE(clipped.max.z == 4);

    // Intersecting an empty box stays empty.
    Box3 none;
    none.intersect(other);
    REQUIRE(none.isEmpty());
}

TEST_CASE("Box3 applyMatrix4, translate, expandByScalar and equals match upstream") {
    Box3 unit(Vector3(-1, -1, -1), Vector3(1, 1, 1));

    Box3 moved = unit.clone();
    Matrix4 translation;
    translation.makeTranslation(5, 0, 0);
    moved.applyMatrix4(translation);
    REQUIRE(moved.min.x == 4);
    REQUIRE(moved.min.y == -1);
    REQUIRE(moved.min.z == -1);
    REQUIRE(moved.max.x == 6);
    REQUIRE(moved.max.y == 1);
    REQUIRE(moved.max.z == 1);

    Box3 translated = unit.clone();
    translated.translate(Vector3(2, 3, 4));
    REQUIRE(translated.min.x == 1);
    REQUIRE(translated.min.y == 2);
    REQUIRE(translated.min.z == 3);

    Box3 grown(Vector3(0, 0, 0), Vector3(1, 1, 1));
    grown.expandByScalar(2);
    REQUIRE(grown.min.x == -2);
    REQUIRE(grown.min.y == -2);
    REQUIRE(grown.min.z == -2);
    REQUIRE(grown.max.x == 3);

    Box3 same(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    REQUIRE(unit.equals(same));
    REQUIRE_FALSE(unit.equals(Box3(Vector3(0, 0, 0), Vector3(4, 4, 4))));
}

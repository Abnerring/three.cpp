#include "catch.hpp"
#include "math/Box3.h"
#include "math/Capsule.h"
#include "math/Vector3.h"

TEST_CASE("Capsule constructor, set, copy and clone keep value semantics", "[capsule]") {
    SECTION("the constructor defaults are start 0, end 1 on y and radius 1") {
        Capsule capsule;
        REQUIRE(capsule.start.x == 0.0);
        REQUIRE(capsule.start.y == 0.0);
        REQUIRE(capsule.start.z == 0.0);
        REQUIRE(capsule.end.x == 0.0);
        REQUIRE(capsule.end.y == 1.0);
        REQUIRE(capsule.end.z == 0.0);
        REQUIRE(capsule.radius == 1.0);
    }

    SECTION("constructor arguments are copied, not aliased") {
        Vector3 start(1.0, 2.0, 3.0), end(4.0, 5.0, 6.0);
        Capsule capsule(start, end, 2.5);
        start.set(9.0, 9.0, 9.0);
        end.set(9.0, 9.0, 9.0);
        REQUIRE(capsule.start.x == 1.0);
        REQUIRE(capsule.start.y == 2.0);
        REQUIRE(capsule.start.z == 3.0);
        REQUIRE(capsule.end.x == 4.0);
        REQUIRE(capsule.end.y == 5.0);
        REQUIRE(capsule.end.z == 6.0);
        REQUIRE(capsule.radius == 2.5);
    }

    SECTION("set copies the values and returns the capsule") {
        Capsule capsule;
        Vector3 start(1.0, 2.0, 3.0), end(4.0, 5.0, 6.0);
        REQUIRE(&capsule.set(start, end, 0.5) == &capsule);
        REQUIRE(capsule.start.x == 1.0);
        REQUIRE(capsule.end.z == 6.0);
        REQUIRE(capsule.radius == 0.5);
        start.set(9.0, 9.0, 9.0);
        end.set(9.0, 9.0, 9.0);
        REQUIRE(capsule.start.x == 1.0);
        REQUIRE(capsule.end.x == 4.0);
    }

    SECTION("copy copies values and returns this") {
        Capsule source(Vector3(1.0, 2.0, 3.0), Vector3(4.0, 5.0, 6.0), 2.0);
        Capsule target;
        REQUIRE(&target.copy(source) == &target);
        REQUIRE(target.start.x == 1.0);
        REQUIRE(target.start.y == 2.0);
        REQUIRE(target.start.z == 3.0);
        REQUIRE(target.end.x == 4.0);
        REQUIRE(target.end.y == 5.0);
        REQUIRE(target.end.z == 6.0);
        REQUIRE(target.radius == 2.0);
        source.start.set(7.0, 7.0, 7.0);
        source.radius = 3.0;
        REQUIRE(target.start.x == 1.0);
        REQUIRE(target.radius == 2.0);
    }

    SECTION("clone starts from the constructor defaults and stays independent") {
        Capsule source(Vector3(1.0, 2.0, 3.0), Vector3(4.0, 5.0, 6.0), 2.0);
        Capsule clone = source.clone();
        REQUIRE(clone.start.x == 1.0);
        REQUIRE(clone.start.y == 2.0);
        REQUIRE(clone.start.z == 3.0);
        REQUIRE(clone.end.x == 4.0);
        REQUIRE(clone.end.z == 6.0);
        REQUIRE(clone.radius == 2.0);
        clone.start.set(7.0, 7.0, 7.0);
        clone.radius = 5.0;
        REQUIRE(source.start.x == 1.0);
        REQUIRE(source.radius == 2.0);
    }
}

TEST_CASE("Capsule getCenter and translate", "[capsule]") {
    SECTION("getCenter is ( end + start ) * 0.5") {
        Capsule capsule(Vector3(1.0, 2.0, 3.0), Vector3(2.0, 4.0, 6.0), 1.0);
        Vector3 target;
        REQUIRE(&capsule.getCenter(target) == &target);
        REQUIRE(target.x == 1.5);
        REQUIRE(target.y == 3.0);
        REQUIRE(target.z == 4.5);

        Capsule defaultCapsule;
        defaultCapsule.getCenter(target);
        REQUIRE(target.x == 0.0);
        REQUIRE(target.y == 0.5);
        REQUIRE(target.z == 0.0);
    }

    SECTION("translate moves both end points and returns the capsule") {
        Capsule capsule(Vector3(1.0, 2.0, 3.0), Vector3(2.0, 4.0, 6.0), 1.0);
        const Vector3 offset(-1.0, 0.5, 2.0);
        REQUIRE(&capsule.translate(offset) == &capsule);
        REQUIRE(capsule.start.x == 0.0);
        REQUIRE(capsule.start.y == 2.5);
        REQUIRE(capsule.start.z == 5.0);
        REQUIRE(capsule.end.x == 1.0);
        REQUIRE(capsule.end.y == 4.5);
        REQUIRE(capsule.end.z == 8.0);
        REQUIRE(capsule.radius == 1.0);
        // The offset itself is not modified.
        REQUIRE(offset.x == -1.0);

        Vector3 center;
        capsule.getCenter(center);
        REQUIRE(center.x == 0.5);
        REQUIRE(center.y == 3.5);
        REQUIRE(center.z == 6.5);
    }
}

TEST_CASE("Capsule intersectsBox applies the strict AABB axis test", "[capsule]") {
    const Capsule unit; // start (0,0,0), end (0,1,0), radius 1

    SECTION("an overlapping box intersects") {
        REQUIRE(unit.intersectsBox(Box3(Vector3(-0.5, -0.5, -0.5), Vector3(0.5, 0.5, 0.5))));
        // Every axis pair of this box overlaps the segment projection inside the radius.
        REQUIRE(unit.intersectsBox(Box3(Vector3(0.5, 0.0, 0.5), Vector3(1.5, 1.0, 1.5))));
    }

    SECTION("a box exactly one radius away does not intersect") {
        // checkAABBAxis uses strict `<` comparisons, so touching at exactly `radius` misses.
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(1.0, 0.0, 0.0), Vector3(2.0, 1.0, 1.0))));
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(-2.0, 0.0, 0.0), Vector3(-1.0, 1.0, 1.0))));
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(-1.0, -2.0, -1.0), Vector3(1.0, -1.0, 1.0))));
        REQUIRE(unit.intersectsBox(Box3(Vector3(0.9999999, 0.0, 0.0), Vector3(2.0, 1.0, 1.0))));
    }

    SECTION("a box far away on any axis pair misses") {
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(5.0, 5.0, 5.0), Vector3(6.0, 6.0, 6.0))));
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(0.0, 0.0, 10.0), Vector3(1.0, 1.0, 11.0))));
        REQUIRE_FALSE(unit.intersectsBox(Box3(Vector3(0.0, 10.0, 0.0), Vector3(1.0, 11.0, 1.0))));
        // An empty Box3 has min = +Infinity and max = -Infinity and never intersects.
        REQUIRE_FALSE(unit.intersectsBox(Box3()));
    }

    SECTION("radius and off-axis segments change the result") {
        const Capsule longCapsule(Vector3(0.0, 0.0, 0.0), Vector3(1.0, 0.0, 0.0), 0.0);
        REQUIRE(longCapsule.intersectsBox(Box3(Vector3(0.25, -1.0, -1.0), Vector3(0.75, 1.0, 1.0))));
        REQUIRE_FALSE(longCapsule.intersectsBox(Box3(Vector3(0.25, 0.5, -1.0), Vector3(0.75, 1.0, 1.0))));

        const Capsule zCapsule(Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 10.0), 0.5);
        REQUIRE(zCapsule.intersectsBox(Box3(Vector3(-1.0, -1.0, 4.0), Vector3(1.0, 1.0, 6.0))));
        REQUIRE_FALSE(zCapsule.intersectsBox(Box3(Vector3(-1.0, 2.0, 4.0), Vector3(1.0, 3.0, 6.0))));

        const Capsule diagonal(Vector3(0.0, 0.0, 0.0), Vector3(2.0, 2.0, 2.0), 0.5);
        REQUIRE(diagonal.intersectsBox(Box3(Vector3(1.0, 1.0, 1.0), Vector3(3.0, 3.0, 3.0))));
    }
}

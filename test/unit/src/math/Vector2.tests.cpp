/**
 * @file   Vector2.tests.cpp
 * @brief  Vector2 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Vector2.h"
#include <type_traits>
#include <utility>

#define EPSILON 1e-6

TEST_CASE("Vector2 exposes native components and bound size aliases", "[properties]") {
    static_assert(std::is_same_v<decltype(Vector2().x), double>);
    static_assert(std::is_same_v<decltype(Vector2().y), double>);
    Vector2 source(1, 2);
    source.x = 3; source.height = 4;
    REQUIRE(source.toArray() == std::vector<double>{3, 4});
    Vector2 copied(source), moved(std::move(source));
    copied.width = 8; moved.height = 9;
    REQUIRE(copied.x == 8);
    REQUIRE(moved.y == 9);
    REQUIRE(source.x == 3);
    REQUIRE(source.y == 4);
    copied = moved;
    copied.width = -5;
    REQUIRE(copied.x == -5);
    REQUIRE(moved.x == 3);
    auto snapshot = copied.width;
    copied.x = 10;
    REQUIRE(double(snapshot) == -5);
    snapshot = 99;
    REQUIRE(copied.x == 10);
}

TEST_CASE("Vector2 Construction")
{
    SECTION("Default constructor")
    {
        Vector2 v;
        REQUIRE(v.x == 0.0);
        REQUIRE(v.y == 0.0);
    }

    SECTION("Parameterized constructor")
    {
        Vector2 v(1.0, 2.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
    }
}

TEST_CASE("Vector2 setters")
{
    SECTION("set()")
    {
        Vector2 v;
        v.set(3.0, 4.0);
        REQUIRE(v.x == 3.0);
        REQUIRE(v.y == 4.0);
    }

    SECTION("setScalar()")
    {
        Vector2 v(1.0, 2.0);
        v.setScalar(5.0);
        REQUIRE(v.x == 5.0);
        REQUIRE(v.y == 5.0);
    }

    SECTION("setX/setY()")
    {
        Vector2 v;
        v.setX(1.0);
        v.setY(2.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
    }

    SECTION("width/height alias")
    {
        Vector2 v;
        v.width = 100.0;
        v.height = 200.0;
        REQUIRE(v.width == 100.0);
        REQUIRE(v.height == 200.0);
    }
}

TEST_CASE("Vector2 copy and clone")
{
    SECTION("copy()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b;
        b.copy(a);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);

        // Ensure it's a true copy
        a.setX(0.0);
        a.setY(-1.0);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
    }

    SECTION("clone()")
    {
        Vector2 a(3.0, 4.0);
        Vector2 b = a.clone();
        REQUIRE(b.x == 3.0);
        REQUIRE(b.y == 4.0);
    }
}

TEST_CASE("Vector2 arithmetic operations")
{
    SECTION("add()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(3.0, 4.0);
        a.add(b);
        REQUIRE(a.x == 4.0);
        REQUIRE(a.y == 6.0);
    }

    SECTION("addVectors()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(3.0, 4.0);
        Vector2 c;
        c.addVectors(a, b);
        REQUIRE(c.x == 4.0);
        REQUIRE(c.y == 6.0);
    }

    SECTION("sub()")
    {
        Vector2 a(5.0, 7.0);
        Vector2 b(2.0, 3.0);
        a.sub(b);
        REQUIRE(a.x == 3.0);
        REQUIRE(a.y == 4.0);
    }

    SECTION("subVectors()")
    {
        Vector2 a(5.0, 7.0);
        Vector2 b(2.0, 3.0);
        Vector2 c;
        c.subVectors(a, b);
        REQUIRE(c.x == 3.0);
        REQUIRE(c.y == 4.0);
    }

    SECTION("multiplyScalar()")
    {
        Vector2 a(2.0, 3.0);
        a.multiplyScalar(3.0);
        REQUIRE(a.x == 6.0);
        REQUIRE(a.y == 9.0);
    }

    SECTION("divideScalar()")
    {
        Vector2 a(6.0, 9.0);
        a.divideScalar(3.0);
        REQUIRE(a.x == 2.0);
        REQUIRE(a.y == 3.0);
    }

    SECTION("negate()")
    {
        Vector2 a(1.0, -2.0);
        a.negate();
        REQUIRE(a.x == -1.0);
        REQUIRE(a.y == 2.0);
    }
}

TEST_CASE("Vector2 dot and cross product")
{
    SECTION("dot()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(3.0, 4.0);
        HIGH_PRECISION result = a.dot(b);
        REQUIRE(result == 1.0 * 3.0 + 2.0 * 4.0);
    }

    SECTION("dot() with perpendicular vectors")
    {
        Vector2 a(1.0, 0.0);
        Vector2 b(0.0, 1.0);
        HIGH_PRECISION result = a.dot(b);
        REQUIRE(result == 0.0);
    }
}

TEST_CASE("Vector2 length operations")
{
    SECTION("length()")
    {
        Vector2 a(3.0, 4.0);
        HIGH_PRECISION len = a.length();
        REQUIRE(len == Approx(5.0).epsilon(EPSILON));
    }

    SECTION("lengthSq()")
    {
        Vector2 a(3.0, 4.0);
        REQUIRE(a.lengthSq() == 25.0);
    }

    SECTION("normalize()")
    {
        Vector2 a(3.0, 4.0);
        a.normalize();
        REQUIRE(a.length() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(a.x == Approx(0.6).epsilon(EPSILON));
        REQUIRE(a.y == Approx(0.8).epsilon(EPSILON));
    }

    SECTION("setLength()")
    {
        Vector2 a(3.0, 4.0);
        a.setLength(10.0);
        REQUIRE(a.length() == Approx(10.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector2 min/max/clamp")
{
    SECTION("min()")
    {
        Vector2 a(5.0, 3.0);
        Vector2 b(2.0, 6.0);
        a.min(b);
        REQUIRE(a.x == 2.0);
        REQUIRE(a.y == 3.0);
    }

    SECTION("max()")
    {
        Vector2 a(5.0, 3.0);
        Vector2 b(2.0, 6.0);
        a.max(b);
        REQUIRE(a.x == 5.0);
        REQUIRE(a.y == 6.0);
    }

    SECTION("clampScalar()")
    {
        Vector2 a(-5.0, 10.0);
        a.clampScalar(0.0, 3.0);
        REQUIRE(a.x == 0.0);
        REQUIRE(a.y == 3.0);
    }
}

TEST_CASE("Vector2 rounding operations")
{
    SECTION("floor()")
    {
        Vector2 a(-0.9, 0.1);
        a.floor();
        REQUIRE(a.x == -1.0);
        REQUIRE(a.y == 0.0);
    }

    SECTION("ceil()")
    {
        Vector2 a(-0.1, 0.9);
        a.ceil();
        REQUIRE(a.x == 0.0);
        REQUIRE(a.y == 1.0);
    }

    SECTION("round()")
    {
        Vector2 a(-0.6, 0.6);
        a.round();
        REQUIRE(a.x == -1.0);
        REQUIRE(a.y == 1.0);
    }

    SECTION("roundToZero()")
    {
        Vector2 a(-0.9, 0.9);
        a.roundToZero();
        REQUIRE(a.x == 0.0);
        REQUIRE(a.y == 0.0);
    }
}

TEST_CASE("Vector2 distance operations")
{
    SECTION("distanceTo()")
    {
        Vector2 a(0.0, 0.0);
        Vector2 b(3.0, 4.0);
        REQUIRE(a.distanceTo(b) == Approx(5.0).epsilon(EPSILON));
    }

    SECTION("distanceToSquared()")
    {
        Vector2 a(0.0, 0.0);
        Vector2 b(3.0, 4.0);
        REQUIRE(a.distanceToSquared(b) == 25.0);
    }

    SECTION("manhattanDistanceTo()")
    {
        Vector2 a(0.0, 0.0);
        Vector2 b(3.0, 4.0);
        REQUIRE(a.manhattanDistanceTo(b) == 7.0);
    }
}

TEST_CASE("Vector2 lerp operations")
{
    SECTION("lerp()")
    {
        Vector2 a(0.0, 0.0);
        Vector2 b(10.0, 20.0);
        a.lerp(b, 0.5);
        REQUIRE(a.x == Approx(5.0).epsilon(EPSILON));
        REQUIRE(a.y == Approx(10.0).epsilon(EPSILON));
    }

    SECTION("lerp() at boundaries")
    {
        Vector2 a(0.0, 0.0);
        Vector2 b(10.0, 20.0);
        Vector2 c = a;
        c.lerp(b, 0.0);
        REQUIRE(c.x == Approx(0.0).epsilon(EPSILON));
        c.lerp(b, 1.0);
        REQUIRE(c.x == Approx(10.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector2 equals and operators")
{
    SECTION("equals()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(1.0, 2.0);
        Vector2 c(1.0, 3.0);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }

    SECTION("equals() with epsilon")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(1.0 + 1e-7, 2.0 + 1e-7);
        REQUIRE(a.equals(b, 1e-6));
    }

    SECTION("operator==()")
    {
        Vector2 a(1.0, 2.0);
        Vector2 b(1.0, 2.0);
        REQUIRE(a == b);
    }

    SECTION("operator[]()")
    {
        Vector2 a(1.0, 2.0);
        REQUIRE(a[0] == 1.0);
        REQUIRE(a[1] == 2.0);
    }
}

TEST_CASE("Vector2 array operations")
{
    SECTION("fromArray()")
    {
        Vector2 v;
        std::vector<HIGH_PRECISION> arr = {1.0, 2.0, 3.0, 4.0};
        v.fromArray(arr);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
    }

    SECTION("fromArray() with offset")
    {
        Vector2 v;
        std::vector<HIGH_PRECISION> arr = {1.0, 2.0, 3.0, 4.0};
        v.fromArray(arr, 2);
        REQUIRE(v.x == 3.0);
        REQUIRE(v.y == 4.0);
    }

    SECTION("toArray()")
    {
        Vector2 v(1.0, 2.0);
        std::vector<HIGH_PRECISION> arr;
        v.toArray(arr);
        REQUIRE(arr[0] == 1.0);
        REQUIRE(arr[1] == 2.0);
    }
}

/**
 * @file   Vector3.tests.cpp
 * @brief  Vector3 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Vector3.h"
#include <type_traits>

TEST_CASE("Vector3 components support native property writes", "[properties]") {
    static_assert(std::is_same_v<decltype(Vector3().x), double>);
    static_assert(std::is_same_v<decltype(Vector3().z), double>);
    Vector3 vector;
    vector.x = 1; vector.y = 2; vector.z = 3;
    vector.x += vector.y;
    REQUIRE(vector.toArray() == std::vector<double>{3, 2, 3});
    auto copied = vector;
    copied.z = 8;
    REQUIRE(vector.z == 3);
    REQUIRE(copied.lengthSq() == 77);
}

#define EPSILON 1e-6

TEST_CASE("Vector3 Construction")
{
    SECTION("Default constructor")
    {
        Vector3 v;
        REQUIRE(v.x == 0.0);
        REQUIRE(v.y == 0.0);
        REQUIRE(v.z == 0.0);
    }

    SECTION("Parameterized constructor")
    {
        Vector3 v(1.0, 2.0, 3.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
    }
}

TEST_CASE("Vector3 setters")
{
    SECTION("set()")
    {
        Vector3 v;
        v.set(1.0, 2.0, 3.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
    }

    SECTION("setX/setY/setZ()")
    {
        Vector3 v;
        v.setX(1.0);
        v.setY(2.0);
        v.setZ(3.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
    }
}

TEST_CASE("Vector3 copy and clone")
{
    SECTION("copy()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        Vector3 b;
        b.copy(a);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
    }

    SECTION("clone()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        Vector3 b = a.clone();
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
    }
}

TEST_CASE("Vector3 arithmetic operations")
{
    SECTION("add()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        Vector3 b(4.0, 5.0, 6.0);
        a.add(b);
        REQUIRE(a.x == 5.0);
        REQUIRE(a.y == 7.0);
        REQUIRE(a.z == 9.0);
    }

    SECTION("sub()")
    {
        Vector3 a(5.0, 7.0, 9.0);
        Vector3 b(2.0, 3.0, 4.0);
        a.sub(b);
        REQUIRE(a.x == 3.0);
        REQUIRE(a.y == 4.0);
        REQUIRE(a.z == 5.0);
    }

    SECTION("multiplyScalar()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        a.multiplyScalar(2.0);
        REQUIRE(a.x == 2.0);
        REQUIRE(a.y == 4.0);
        REQUIRE(a.z == 6.0);
    }

    SECTION("divideScalar()")
    {
        Vector3 a(2.0, 4.0, 6.0);
        a.divideScalar(2.0);
        REQUIRE(a.x == 1.0);
        REQUIRE(a.y == 2.0);
        REQUIRE(a.z == 3.0);
    }

    SECTION("negate()")
    {
        Vector3 a(1.0, -2.0, 3.0);
        a.negate();
        REQUIRE(a.x == -1.0);
        REQUIRE(a.y == 2.0);
        REQUIRE(a.z == -3.0);
    }
}

TEST_CASE("Vector3 dot and cross product")
{
    SECTION("dot()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        Vector3 b(4.0, 5.0, 6.0);
        HIGH_PRECISION result = a.dot(b);
        REQUIRE(result == 1.0 * 4.0 + 2.0 * 5.0 + 3.0 * 6.0);
    }

    SECTION("cross()")
    {
        Vector3 a(1.0, 0.0, 0.0);
        Vector3 b(0.0, 1.0, 0.0);
        a.cross(b);
        REQUIRE(a.x == 0.0);
        REQUIRE(a.y == 0.0);
        REQUIRE(a.z == 1.0);
    }

    SECTION("crossVectors()")
    {
        Vector3 a(1.0, 0.0, 0.0);
        Vector3 b(0.0, 1.0, 0.0);
        Vector3 c;
        c.crossVectors(a, b);
        REQUIRE(c.x == 0.0);
        REQUIRE(c.y == 0.0);
        REQUIRE(c.z == 1.0);
    }
}

TEST_CASE("Vector3 length operations")
{
    SECTION("length()")
    {
        Vector3 a(1.0, 2.0, 2.0);
        HIGH_PRECISION len = a.length();
        REQUIRE(len == Approx(3.0).epsilon(EPSILON));
    }

    SECTION("lengthSq()")
    {
        Vector3 a(1.0, 2.0, 2.0);
        REQUIRE(a.lengthSq() == 9.0);
    }

    SECTION("normalize()")
    {
        Vector3 a(0.0, 3.0, 4.0);
        a.normalize();
        REQUIRE(a.length() == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("setLength()")
    {
        Vector3 a(0.0, 3.0, 4.0);
        a.setLength(10.0);
        REQUIRE(a.length() == Approx(10.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector3 clamp operations")
{
    SECTION("min()")
    {
        Vector3 a(5.0, 3.0, 7.0);
        Vector3 b(4.0, 6.0, 5.0);
        a.min(b);
        REQUIRE(a.x == 4.0);
        REQUIRE(a.y == 3.0);
        REQUIRE(a.z == 5.0);
    }

    SECTION("max()")
    {
        Vector3 a(5.0, 3.0, 7.0);
        Vector3 b(4.0, 6.0, 5.0);
        a.max(b);
        REQUIRE(a.x == 5.0);
        REQUIRE(a.y == 6.0);
        REQUIRE(a.z == 7.0);
    }

    SECTION("clampScalar()")
    {
        Vector3 a(-5.0, 10.0, 0.0);
        a.clampScalar(0.0, 3.0);
        REQUIRE(a.x == 0.0);
        REQUIRE(a.y == 3.0);
        REQUIRE(a.z == 0.0);
    }
}

TEST_CASE("Vector3 distance operations")
{
    SECTION("distanceTo()")
    {
        Vector3 a(0.0, 0.0, 0.0);
        Vector3 b(1.0, 2.0, 2.0);
        REQUIRE(a.distanceTo(b) == Approx(3.0).epsilon(EPSILON));
    }

    SECTION("distanceToSquared()")
    {
        Vector3 a(0.0, 0.0, 0.0);
        Vector3 b(1.0, 2.0, 2.0);
        REQUIRE(a.distanceToSquared(b) == 9.0);
    }
}

TEST_CASE("Vector3 lerp operations")
{
    SECTION("lerp()")
    {
        Vector3 a(0.0, 0.0, 0.0);
        Vector3 b(10.0, 20.0, 30.0);
        a.lerp(b, 0.5);
        REQUIRE(a.x == Approx(5.0).epsilon(EPSILON));
        REQUIRE(a.y == Approx(10.0).epsilon(EPSILON));
        REQUIRE(a.z == Approx(15.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector3 equals")
{
    SECTION("equals()")
    {
        Vector3 a(1.0, 2.0, 3.0);
        Vector3 b(1.0, 2.0, 3.0);
        Vector3 c(1.0, 2.0, 4.0);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

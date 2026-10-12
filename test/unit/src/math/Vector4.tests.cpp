/**
 * @file   Vector4.tests.cpp
 * @brief  Vector4 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Vector4.h"
#include <type_traits>
#include <utility>

TEST_CASE("Vector4 native components and size aliases retain their receiver", "[properties]") {
    static_assert(std::is_same_v<decltype(Vector4().w), double>);
    Vector4 vector(1, 2, 3, 4);
    vector.width = 5; vector.height = 6;
    REQUIRE(vector.toArray() == std::vector<double>{1, 2, 5, 6});
    Vector4 copied(vector), moved(std::move(vector));
    copied.z = 8;
    REQUIRE(double(copied.width) == 8);
    REQUIRE(double(vector.width) == 5);
    moved.height = 9;
    REQUIRE(moved.w == 9);
    REQUIRE(vector.w == 6);
    copied = moved;
    copied.width = -2;
    REQUIRE(copied.z == -2);
    REQUIRE(moved.z == 5);
}

#define EPSILON 1e-6

TEST_CASE("Vector4 Construction")
{
    SECTION("Default constructor")
    {
        Vector4 v;
        REQUIRE(v.x == 0.0);
        REQUIRE(v.y == 0.0);
        REQUIRE(v.z == 0.0);
        REQUIRE(v.w == 1.0);
    }

    SECTION("Parameterized constructor")
    {
        Vector4 v(1.0, 2.0, 3.0, 4.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
        REQUIRE(v.w == 4.0);
    }
}

TEST_CASE("Vector4 setters")
{
    SECTION("set()")
    {
        Vector4 v;
        v.set(1.0, 2.0, 3.0, 4.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
        REQUIRE(v.w == 4.0);
    }

    SECTION("setScalar()")
    {
        Vector4 v;
        v.setScalar(5.0);
        REQUIRE(v.x == 5.0);
        REQUIRE(v.y == 5.0);
        REQUIRE(v.z == 5.0);
        REQUIRE(v.w == 5.0);
    }

    SECTION("setX/setY/setZ/setW()")
    {
        Vector4 v;
        v.setX(1.0).setY(2.0).setZ(3.0).setW(4.0);
        REQUIRE(v.x == 1.0);
        REQUIRE(v.y == 2.0);
        REQUIRE(v.z == 3.0);
        REQUIRE(v.w == 4.0);
    }
}

TEST_CASE("Vector4 copy")
{
    SECTION("copy()")
    {
        Vector4 a(1.0, 2.0, 3.0, 4.0);
        Vector4 b;
        b.copy(a);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
        REQUIRE(b.w == 4.0);
    }
}

TEST_CASE("Vector4 arithmetic operations")
{
    SECTION("add()")
    {
        Vector4 a(1.0, 2.0, 3.0, 4.0);
        Vector4 b(1.0, 1.0, 1.0, 1.0);
        a.add(b);
        REQUIRE(a.x == 2.0);
        REQUIRE(a.y == 3.0);
        REQUIRE(a.z == 4.0);
        REQUIRE(a.w == 5.0);
    }

    SECTION("sub()")
    {
        Vector4 a(5.0, 6.0, 7.0, 8.0);
        Vector4 b(1.0, 2.0, 3.0, 4.0);
        a.sub(b);
        REQUIRE(a.x == 4.0);
        REQUIRE(a.y == 4.0);
        REQUIRE(a.z == 4.0);
        REQUIRE(a.w == 4.0);
    }

    SECTION("multiplyScalar()")
    {
        Vector4 a(1.0, 2.0, 3.0, 4.0);
        a.multiplyScalar(2.0);
        REQUIRE(a.x == 2.0);
        REQUIRE(a.y == 4.0);
        REQUIRE(a.z == 6.0);
        REQUIRE(a.w == 8.0);
    }

    SECTION("divideScalar()")
    {
        Vector4 a(2.0, 4.0, 6.0, 8.0);
        a.divideScalar(2.0);
        REQUIRE(a.x == 1.0);
        REQUIRE(a.y == 2.0);
        REQUIRE(a.z == 3.0);
        REQUIRE(a.w == 4.0);
    }

    SECTION("negate()")
    {
        Vector4 a(1.0, -2.0, 3.0, -4.0);
        a.negate();
        REQUIRE(a.x == -1.0);
        REQUIRE(a.y == 2.0);
        REQUIRE(a.z == -3.0);
        REQUIRE(a.w == 4.0);
    }
}

TEST_CASE("Vector4 dot product")
{
    SECTION("dot()")
    {
        Vector4 a(1.0, 2.0, 3.0, 4.0);
        Vector4 b(4.0, 3.0, 2.0, 1.0);
        HIGH_PRECISION result = a.dot(b);
        REQUIRE(result == 1.0 * 4.0 + 2.0 * 3.0 + 3.0 * 2.0 + 4.0 * 1.0);
    }
}

TEST_CASE("Vector4 length operations")
{
    SECTION("length()")
    {
        Vector4 a(1.0, 0.0, 0.0, 0.0);
        REQUIRE(a.length() == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("lengthSq()")
    {
        Vector4 a(1.0, 2.0, 2.0, 4.0);
        REQUIRE(a.lengthSq() == 25.0);
    }

    SECTION("normalize()")
    {
        Vector4 a(0.0, 3.0, 0.0, 4.0);
        a.normalize();
        REQUIRE(a.length() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector4 clamp operations")
{
    SECTION("min()")
    {
        Vector4 a(5.0, 3.0, 7.0, 2.0);
        Vector4 b(4.0, 6.0, 5.0, 8.0);
        a.min(b);
        REQUIRE(a.x == 4.0);
        REQUIRE(a.y == 3.0);
        REQUIRE(a.z == 5.0);
        REQUIRE(a.w == 2.0);
    }

    SECTION("max()")
    {
        Vector4 a(5.0, 3.0, 7.0, 2.0);
        Vector4 b(4.0, 6.0, 5.0, 8.0);
        a.max(b);
        REQUIRE(a.x == 5.0);
        REQUIRE(a.y == 6.0);
        REQUIRE(a.z == 7.0);
        REQUIRE(a.w == 8.0);
    }
}

TEST_CASE("Vector4 lerp")
{
    SECTION("lerp()")
    {
        Vector4 a(0.0, 0.0, 0.0, 0.0);
        Vector4 b(10.0, 20.0, 30.0, 40.0);
        a.lerp(b, 0.5);
        REQUIRE(a.x == Approx(5.0).epsilon(EPSILON));
        REQUIRE(a.y == Approx(10.0).epsilon(EPSILON));
        REQUIRE(a.z == Approx(15.0).epsilon(EPSILON));
        REQUIRE(a.w == Approx(20.0).epsilon(EPSILON));
    }
}

TEST_CASE("Vector4 equals")
{
    SECTION("equals()")
    {
        Vector4 a(1.0, 2.0, 3.0, 4.0);
        Vector4 b(1.0, 2.0, 3.0, 4.0);
        Vector4 c(1.0, 2.0, 3.0, 5.0);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

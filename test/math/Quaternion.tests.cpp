/**
 * @file   Quaternion.tests.cpp
 * @brief  Quaternion 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#define EPSILON 1e-6

TEST_CASE("Quaternion Construction")
{
    SECTION("Default constructor")
    {
        Quaternion q;
        REQUIRE(q.x() == 0.0);
        REQUIRE(q.y() == 0.0);
        REQUIRE(q.z() == 0.0);
        REQUIRE(q.w() == 1.0);
    }

    SECTION("Parameterized constructor")
    {
        Quaternion q(1.0, 2.0, 3.0, 4.0);
        REQUIRE(q.x() == 1.0);
        REQUIRE(q.y() == 2.0);
        REQUIRE(q.z() == 3.0);
        REQUIRE(q.w() == 4.0);
    }
}

TEST_CASE("Quaternion setters")
{
    SECTION("set()")
    {
        Quaternion q;
        q.set(1.0, 2.0, 3.0, 4.0);
        REQUIRE(q.x() == 1.0);
        REQUIRE(q.y() == 2.0);
        REQUIRE(q.z() == 3.0);
        REQUIRE(q.w() == 4.0);
    }

    SECTION("setScalar()")
    {
        Quaternion q;
        q.setScalar(5.0);
        REQUIRE(q.x() == 5.0);
        REQUIRE(q.y() == 5.0);
        REQUIRE(q.z() == 5.0);
        REQUIRE(q.w() == 5.0);
    }

    SECTION("setX/setY/setZ/setW()")
    {
        Quaternion q;
        q.setX(1.0).setY(2.0).setZ(3.0).setW(4.0);
        REQUIRE(q.x() == 1.0);
        REQUIRE(q.y() == 2.0);
        REQUIRE(q.z() == 3.0);
        REQUIRE(q.w() == 4.0);
    }
}

TEST_CASE("Quaternion copy")
{
    SECTION("copy()")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b;
        b.copy(a);
        REQUIRE(b.x() == 1.0);
        REQUIRE(b.y() == 2.0);
        REQUIRE(b.z() == 3.0);
        REQUIRE(b.w() == 4.0);
    }

    SECTION("clone()")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b = a.clone();
        REQUIRE(b.x() == 1.0);
        REQUIRE(b.y() == 2.0);
    }
}

TEST_CASE("Quaternion normalize")
{
    SECTION("normalize non-unit")
    {
        Quaternion q(2.0, 0.0, 0.0, 0.0);
        q.normalize();
        REQUIRE(q.length() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(q.x() == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("normalize unit")
    {
        Quaternion q(0.707, 0.0, 0.0, 0.707);
        q.normalize();
        REQUIRE(q.length() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Quaternion conjugate")
{
    SECTION("conjugate")
    {
        Quaternion q(1.0, 2.0, 3.0, 4.0);
        q.conjugate();
        REQUIRE(q.x() == -1.0);
        REQUIRE(q.y() == -2.0);
        REQUIRE(q.z() == -3.0);
        REQUIRE(q.w() == 4.0);
    }
}

TEST_CASE("Quaternion dot and length")
{
    SECTION("dot()")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b(4.0, 3.0, 2.0, 1.0);
        HIGH_PRECISION result = a.dot(b);
        REQUIRE(result == 1.0 * 4.0 + 2.0 * 3.0 + 3.0 * 2.0 + 4.0 * 1.0);
    }

    SECTION("length()")
    {
        Quaternion q(1.0, 0.0, 0.0, 0.0);
        REQUIRE(q.length() == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("lengthSq()")
    {
        Quaternion q(2.0, 0.0, 0.0, 0.0);
        REQUIRE(q.lengthSq() == Approx(4.0).epsilon(EPSILON));
    }
}

TEST_CASE("Quaternion multiply")
{
    SECTION("multiply identity")
    {
        Quaternion q;
        Quaternion identity;
        q.multiply(identity);
        REQUIRE(q.w() == 1.0);
    }
}

TEST_CASE("Quaternion slerp")
{
    SECTION("slerp at boundaries")
    {
        Quaternion q1(0.0, 0.0, 0.0, 1.0);
        Quaternion q2(0.0, 0.0, 0.0, 1.0);
        Quaternion result = q1;
        result.slerp(q2, 0.0);
        REQUIRE(result.w() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Quaternion lerp")
{
    SECTION("lerp")
    {
        Quaternion q1(0.0, 0.0, 0.0, 1.0);
        Quaternion q2(1.0, 0.0, 0.0, 0.0);
        Quaternion result = q1;
        result.lerp(q2, 0.5);
        REQUIRE(result.x() > 0.0);
        REQUIRE(result.w() > 0.0);
    }
}

TEST_CASE("Quaternion scalar operations")
{
    SECTION("multiplyScalar()")
    {
        Quaternion q(1.0, 2.0, 3.0, 4.0);
        q.multiplyScalar(2.0);
        REQUIRE(q.x() == 2.0);
        REQUIRE(q.y() == 4.0);
        REQUIRE(q.z() == 6.0);
        REQUIRE(q.w() == 8.0);
    }

    SECTION("divideScalar()")
    {
        Quaternion q(2.0, 4.0, 6.0, 8.0);
        q.divideScalar(2.0);
        REQUIRE(q.x() == 1.0);
        REQUIRE(q.y() == 2.0);
        REQUIRE(q.z() == 3.0);
        REQUIRE(q.w() == 4.0);
    }
}

TEST_CASE("Quaternion equals")
{
    SECTION("equals")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b(1.0, 2.0, 3.0, 4.0);
        Quaternion c(1.0, 2.0, 3.0, 5.0);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

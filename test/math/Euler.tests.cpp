/**
 * @file   Euler.tests.cpp
 * @brief  Euler 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Euler.h"
#include "math/Quaternion.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/MathUtils.h"

#define EPSILON 1e-6

TEST_CASE("Euler Construction")
{
    SECTION("Default constructor")
    {
        Euler e;
        REQUIRE(e.x() == 0.0);
        REQUIRE(e.y() == 0.0);
        REQUIRE(e.z() == 0.0);
        REQUIRE(e.order() == EulerOrder::XYZ);
    }

    SECTION("Parameterized constructor")
    {
        Euler e(1.0, 2.0, 3.0, EulerOrder::YXZ);
        REQUIRE(e.x() == 1.0);
        REQUIRE(e.y() == 2.0);
        REQUIRE(e.z() == 3.0);
        REQUIRE(e.order() == EulerOrder::YXZ);
    }
}

TEST_CASE("Euler setters")
{
    SECTION("set()")
    {
        Euler e;
        e.set(1.0, 2.0, 3.0, EulerOrder::ZYX);
        REQUIRE(e.x() == 1.0);
        REQUIRE(e.y() == 2.0);
        REQUIRE(e.z() == 3.0);
        REQUIRE(e.order() == EulerOrder::ZYX);
    }

    SECTION("setX/setY/setZ()")
    {
        Euler e;
        e.setX(1.0).setY(2.0).setZ(3.0);
        REQUIRE(e.x() == 1.0);
        REQUIRE(e.y() == 2.0);
        REQUIRE(e.z() == 3.0);
    }

    SECTION("setOrder()")
    {
        Euler e;
        e.setOrder(EulerOrder::YZX);
        REQUIRE(e.order() == EulerOrder::YZX);
    }
}

TEST_CASE("Euler copy")
{
    SECTION("copy()")
    {
        Euler a(1.0, 2.0, 3.0, EulerOrder::XZY);
        Euler b;
        b.copy(a);
        REQUIRE(b.x() == 1.0);
        REQUIRE(b.y() == 2.0);
        REQUIRE(b.z() == 3.0);
        REQUIRE(b.order() == EulerOrder::XZY);
    }

    SECTION("clone()")
    {
        Euler a(1.0, 2.0, 3.0, EulerOrder::ZXY);
        Euler b = a.clone();
        REQUIRE(b.x() == 1.0);
        REQUIRE(b.y() == 2.0);
        REQUIRE(b.z() == 3.0);
    }
}

TEST_CASE("Euler conversion")
{
    SECTION("setFromQuaternion")
    {
        Euler e;
        Quaternion q;
        q.setFromAxisAngle(Vector3(0.0, 1.0, 0.0), MathUtils::PI / 2);
        e.setFromQuaternion(q, EulerOrder::XYZ);
        REQUIRE(e.x() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(e.y() == Approx(MathUtils::PI / 2).epsilon(EPSILON));
    }

    SECTION("toQuaternion")
    {
        Euler e(MathUtils::PI / 2, 0, 0);
        Quaternion q;
        e.toQuaternion(q);
        REQUIRE(q.length() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Euler equals")
{
    SECTION("equals")
    {
        Euler a(1.0, 2.0, 3.0, EulerOrder::XYZ);
        Euler b(1.0, 2.0, 3.0, EulerOrder::XYZ);
        Euler c(1.0, 2.0, 3.0, EulerOrder::YXZ);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

TEST_CASE("Euler getOrderString")
{
    SECTION("getOrderString")
    {
        Euler e(0, 0, 0, EulerOrder::XYZ);
        REQUIRE(e.getOrderString() == "XYZ");

        e.setOrder(EulerOrder::ZYX);
        REQUIRE(e.getOrderString() == "ZYX");
    }
}

TEST_CASE("Euler rotation orders")
{
    SECTION("All rotation orders can be created")
    {
        Euler e1(0, 0, 0, EulerOrder::XYZ);
        Euler e2(0, 0, 0, EulerOrder::XZY);
        Euler e3(0, 0, 0, EulerOrder::YXZ);
        Euler e4(0, 0, 0, EulerOrder::YZX);
        Euler e5(0, 0, 0, EulerOrder::ZXY);
        Euler e6(0, 0, 0, EulerOrder::ZYX);

        REQUIRE(e1.order() == EulerOrder::XYZ);
        REQUIRE(e2.order() == EulerOrder::XZY);
        REQUIRE(e3.order() == EulerOrder::YXZ);
        REQUIRE(e4.order() == EulerOrder::YZX);
        REQUIRE(e5.order() == EulerOrder::ZXY);
        REQUIRE(e6.order() == EulerOrder::ZYX);
    }
}

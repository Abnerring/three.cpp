/**
 * @file   Matrix4.tests.cpp
 * @brief  Matrix4 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/MathUtils.h"

#define EPSILON 1e-6

TEST_CASE("Matrix4 Construction")
{
    SECTION("Default constructor - identity")
    {
        Matrix4 m;
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[5] == 1.0);
        REQUIRE(m[10] == 1.0);
        REQUIRE(m[15] == 1.0);
    }
}

TEST_CASE("Matrix4 setters")
{
    SECTION("set()")
    {
        Matrix4 m;
        m.set(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[15] == 1.0);
    }

    SECTION("identity()")
    {
        Matrix4 m(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2);
        m.identity();
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[5] == 1.0);
        REQUIRE(m[10] == 1.0);
        REQUIRE(m[15] == 1.0);
    }
}

TEST_CASE("Matrix4 copy")
{
    SECTION("copy()")
    {
        Matrix4 a;
        a.set(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        Matrix4 b;
        b.copy(a);
        REQUIRE(b[0] == 1.0);
        REQUIRE(b[15] == 16.0);
    }
}

TEST_CASE("Matrix4 multiply")
{
    SECTION("multiplyMatrices")
    {
        Matrix4 a;
        a.identity();
        Matrix4 b;
        b.makeScale(2.0, 2.0, 2.0);
        Matrix4 result;
        result.multiplyMatrices(a, b);
        REQUIRE(result[0] == Approx(2.0).epsilon(EPSILON));
        REQUIRE(result[5] == Approx(2.0).epsilon(EPSILON));
        REQUIRE(result[10] == Approx(2.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 translation")
{
    SECTION("makeTranslation(x, y, z)")
    {
        Matrix4 m;
        m.makeTranslation(1.0, 2.0, 3.0);
        REQUIRE(m[12] == 1.0);
        REQUIRE(m[13] == 2.0);
        REQUIRE(m[14] == 3.0);
    }

    SECTION("makeTranslation(Vector3)")
    {
        Vector3 v(1.0, 2.0, 3.0);
        Matrix4 m;
        m.makeTranslation(v);
        REQUIRE(m[12] == 1.0);
        REQUIRE(m[13] == 2.0);
        REQUIRE(m[14] == 3.0);
    }
}

TEST_CASE("Matrix4 rotation")
{
    SECTION("makeRotationX")
    {
        Matrix4 m;
        m.makeRotationX(MathUtils::PI / 2);
        REQUIRE(m[0] == Approx(1.0).epsilon(EPSILON));
        REQUIRE(m[5] == Approx(0.0).epsilon(EPSILON));
        REQUIRE(m[6] == Approx(-1.0).epsilon(EPSILON));
    }

    SECTION("makeRotationY")
    {
        Matrix4 m;
        m.makeRotationY(MathUtils::PI / 2);
        REQUIRE(m[0] == Approx(0.0).epsilon(EPSILON));
        REQUIRE(m[5] == Approx(1.0).epsilon(EPSILON));
        REQUIRE(m[2] == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("makeRotationZ")
    {
        Matrix4 m;
        m.makeRotationZ(MathUtils::PI / 2);
        REQUIRE(m[0] == Approx(0.0).epsilon(EPSILON));
        REQUIRE(m[5] == Approx(0.0).epsilon(EPSILON));
        REQUIRE(m[1] == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 scale")
{
    SECTION("makeScale")
    {
        Matrix4 m;
        m.makeScale(2.0, 3.0, 4.0);
        REQUIRE(m[0] == Approx(2.0).epsilon(EPSILON));
        REQUIRE(m[5] == Approx(3.0).epsilon(EPSILON));
        REQUIRE(m[10] == Approx(4.0).epsilon(EPSILON));
        REQUIRE(m[15] == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 determinant")
{
    SECTION("determinant of identity")
    {
        Matrix4 m;
        REQUIRE(m.determinant() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 transpose")
{
    SECTION("transpose identity")
    {
        Matrix4 m;
        m.transpose();
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[1] == 0.0);
    }
}

TEST_CASE("Matrix4 invert")
{
    SECTION("invert identity")
    {
        Matrix4 m;
        m.invert();
        REQUIRE(m[0] == Approx(1.0).epsilon(EPSILON));
        REQUIRE(m[5] == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 lookAt")
{
    SECTION("lookAt")
    {
        Matrix4 m;
        Vector3 eye(0.0, 0.0, 5.0);
        Vector3 target(0.0, 0.0, 0.0);
        Vector3 up(0.0, 1.0, 0.0);
        m.lookAt(eye, target, up);
        REQUIRE(m[14] == Approx(-5.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix4 clone")
{
    SECTION("clone")
    {
        Matrix4 a;
        a.makeTranslation(1.0, 2.0, 3.0);
        Matrix4 b = a.clone();
        REQUIRE(b[12] == 1.0);
        REQUIRE(b[13] == 2.0);
        REQUIRE(b[14] == 3.0);
    }
}

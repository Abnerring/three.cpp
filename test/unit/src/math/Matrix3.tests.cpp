/**
 * @file   Matrix3.tests.cpp
 * @brief  Matrix3 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Matrix3.h"
#include "math/Vector2.h"

#define EPSILON 1e-6

TEST_CASE("Matrix3 Construction")
{
    SECTION("Default constructor - identity")
    {
        Matrix3 m;
        auto e = m.elements();
        REQUIRE(e[0] == 1.0);
        REQUIRE(e[4] == 1.0);
        REQUIRE(e[8] == 1.0);
    }

    SECTION("Parameterized constructor")
    {
        Matrix3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[4] == 5.0);
        REQUIRE(m[8] == 9.0);
    }
}

TEST_CASE("Matrix3 setters")
{
    SECTION("set()")
    {
        Matrix3 m;
        m.set(1, 2, 3, 4, 5, 6, 7, 8, 9);
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[8] == 9.0);
    }

    SECTION("identity()")
    {
        Matrix3 m(2, 3, 4, 5, 6, 7, 8, 9, 10);
        m.identity();
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[4] == 1.0);
        REQUIRE(m[8] == 1.0);
    }
}

TEST_CASE("Matrix3 copy")
{
    SECTION("copy()")
    {
        Matrix3 a(1, 2, 3, 4, 5, 6, 7, 8, 9);
        Matrix3 b;
        b.copy(a);
        REQUIRE(b[0] == 1.0);
        REQUIRE(b[8] == 9.0);
    }
}

TEST_CASE("Matrix3 multiply")
{
    SECTION("multiplyMatrices")
    {
        Matrix3 a;
        a.identity();
        Matrix3 b(2, 0, 0, 0, 2, 0, 0, 0, 2);
        Matrix3 result;
        result.multiplyMatrices(a, b);
        REQUIRE(result[0] == 2.0);
        REQUIRE(result[4] == 2.0);
        REQUIRE(result[8] == 2.0);
    }
}

TEST_CASE("Matrix3 transpose")
{
    SECTION("transpose identity")
    {
        Matrix3 m;
        m.transpose();
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[1] == 0.0);
    }
}

TEST_CASE("Matrix3 determinant")
{
    SECTION("determinant of identity")
    {
        Matrix3 m;
        REQUIRE(m.determinant() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Matrix3 invert")
{
    SECTION("invert identity")
    {
        Matrix3 m;
        m.invert();
        REQUIRE(m[0] == 1.0);
        REQUIRE(m[4] == 1.0);
        REQUIRE(m[8] == 1.0);
    }
}

TEST_CASE("Matrix3 equals")
{
    SECTION("equals with same matrices")
    {
        Matrix3 a(1, 2, 3, 4, 5, 6, 7, 8, 9);
        Matrix3 b(1, 2, 3, 4, 5, 6, 7, 8, 9);
        REQUIRE(a.equals(b));
    }

    SECTION("equals with different matrices")
    {
        Matrix3 a(1, 2, 3, 4, 5, 6, 7, 8, 9);
        Matrix3 b(9, 8, 7, 6, 5, 4, 3, 2, 1);
        REQUIRE_FALSE(a.equals(b));
    }
}

TEST_CASE("Matrix3 clone")
{
    SECTION("clone")
    {
        Matrix3 a(1, 2, 3, 4, 5, 6, 7, 8, 9);
        Matrix3 b = a.clone();
        REQUIRE(b[0] == 1.0);
        REQUIRE(b[8] == 9.0);
    }
}

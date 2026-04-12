/**
 * @file   Color.tests.cpp
 * @brief  Color 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Color.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

#define EPSILON 1e-6

TEST_CASE("Color Construction")
{
    SECTION("Default constructor - white")
    {
        Color c;
        REQUIRE(c.r() == 1.0);
        REQUIRE(c.g() == 1.0);
        REQUIRE(c.b() == 1.0);
    }

    SECTION("RGB constructor")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.r() == 1.0);
        REQUIRE(c.g() == 0.0);
        REQUIRE(c.b() == 0.0);
    }

    SECTION("Hex constructor")
    {
        Color c(0xff0000);
        REQUIRE(c.r() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.g() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(c.b() == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color setters")
{
    SECTION("set()")
    {
        Color c;
        c.set(0.5, 0.5, 0.5);
        REQUIRE(c.r() == 0.5);
        REQUIRE(c.g() == 0.5);
        REQUIRE(c.b() == 0.5);
    }

    SECTION("setScalar()")
    {
        Color c;
        c.setScalar(0.5);
        REQUIRE(c.r() == 0.5);
        REQUIRE(c.g() == 0.5);
        REQUIRE(c.b() == 0.5);
    }

    SECTION("setHex()")
    {
        Color c;
        c.setHex(0x00ff00);
        REQUIRE(c.r() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(c.g() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.b() == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("setHSL()")
    {
        Color c;
        c.setHSL(0.0, 1.0, 0.5);  // Red
        REQUIRE(c.r() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color copy")
{
    SECTION("copy()")
    {
        Color a(0.1, 0.2, 0.3);
        Color b;
        b.copy(a);
        REQUIRE(b.r() == Approx(0.1).epsilon(EPSILON));
        REQUIRE(b.g() == Approx(0.2).epsilon(EPSILON));
        REQUIRE(b.b() == Approx(0.3).epsilon(EPSILON));
    }

    SECTION("clone()")
    {
        Color a(0.3, 0.4, 0.5);
        Color b = a.clone();
        REQUIRE(b.r() == Approx(0.3).epsilon(EPSILON));
    }
}

TEST_CASE("Color arithmetic operations")
{
    SECTION("add()")
    {
        Color c(0.1, 0.2, 0.3);
        Color other(0.1, 0.1, 0.1);
        c.add(other);
        REQUIRE(c.r() == Approx(0.2).epsilon(EPSILON));
        REQUIRE(c.g() == Approx(0.3).epsilon(EPSILON));
        REQUIRE(c.b() == Approx(0.4).epsilon(EPSILON));
    }

    SECTION("sub()")
    {
        Color c(0.5, 0.5, 0.5);
        Color other(0.1, 0.1, 0.1);
        c.sub(other);
        REQUIRE(c.r() == Approx(0.4).epsilon(EPSILON));
    }

    SECTION("multiplyScalar()")
    {
        Color c(0.5, 0.5, 0.5);
        c.multiplyScalar(2.0);
        REQUIRE(c.r() == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("divideScalar()")
    {
        Color c(1.0, 1.0, 1.0);
        c.divideScalar(2.0);
        REQUIRE(c.r() == Approx(0.5).epsilon(EPSILON));
    }
}

TEST_CASE("Color lerp")
{
    SECTION("lerp()")
    {
        Color a(0.0, 0.0, 0.0);
        Color b(1.0, 1.0, 1.0);
        a.lerp(b, 0.5);
        REQUIRE(a.r() == Approx(0.5).epsilon(EPSILON));
        REQUIRE(a.g() == Approx(0.5).epsilon(EPSILON));
        REQUIRE(a.b() == Approx(0.5).epsilon(EPSILON));
    }

    SECTION("lerp at boundaries")
    {
        Color a(0.0, 0.0, 0.0);
        Color b(1.0, 1.0, 1.0);
        Color c = a;
        c.lerp(b, 0.0);
        REQUIRE(c.r() == Approx(0.0).epsilon(EPSILON));
        c.lerp(b, 1.0);
        REQUIRE(c.r() == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color equals")
{
    SECTION("equals()")
    {
        Color a(0.1, 0.2, 0.3);
        Color b(0.1, 0.2, 0.3);
        Color c(0.1, 0.2, 0.4);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

TEST_CASE("Color getHex")
{
    SECTION("getHex()")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.getHex() == 0xff0000);
    }
}

TEST_CASE("Color getHexString")
{
    SECTION("getHexString()")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.getHexString() == "#ff0000");
    }
}

TEST_CASE("Color getHSL")
{
    SECTION("getHSL()")
    {
        Color c(1.0, 0.0, 0.0);  // Red
        Vector3 hsl;
        c.getHSL(hsl);
        REQUIRE(hsl.x() == Approx(0.0).epsilon(EPSILON));  // Hue
        REQUIRE(hsl.y() == Approx(1.0).epsilon(EPSILON));  // Saturation
        REQUIRE(hsl.z() == Approx(0.5).epsilon(EPSILON));  // Lightness
    }
}

TEST_CASE("Color offsetHSL")
{
    SECTION("offsetHSL()")
    {
        Color c(1.0, 0.0, 0.0);  // Red
        c.offsetHSL(0.0, 0.0, 0.2);  // Increase lightness
        Vector3 hsl;
        c.getHSL(hsl);
        REQUIRE(hsl.z() == Approx(0.7).epsilon(EPSILON));
    }
}

TEST_CASE("Color fromVector3")
{
    SECTION("fromVector3()")
    {
        Color c;
        Vector3 v(1.0, 0.5, 0.0);
        c.fromVector3(v);
        REQUIRE(c.r() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.g() == Approx(0.5).epsilon(EPSILON));
        REQUIRE(c.b() == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color toVector3")
{
    SECTION("toVector3()")
    {
        Color c(1.0, 0.5, 0.0);
        Vector3 v;
        c.toVector3(v);
        REQUIRE(v.x() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(v.y() == Approx(0.5).epsilon(EPSILON));
        REQUIRE(v.z() == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color toVector4")
{
    SECTION("toVector4()")
    {
        Color c(1.0, 0.5, 0.0);
        Vector4 v;
        c.toVector4(v);
        REQUIRE(v.x() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(v.y() == Approx(0.5).epsilon(EPSILON));
        REQUIRE(v.z() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(v.w() == Approx(1.0).epsilon(EPSILON));
    }
}

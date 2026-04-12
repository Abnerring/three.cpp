/**
 * @file   MathUtils.tests.cpp
 * @brief  MathUtils 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/MathUtils.h"

#define EPSILON 1e-6

TEST_CASE("MathUtils clamp")
{
    SECTION("clamp within range")
    {
        REQUIRE(MathUtils::clamp(5.0, 0.0, 10.0) == 5.0);
    }

    SECTION("clamp below minimum")
    {
        REQUIRE(MathUtils::clamp(-5.0, 0.0, 10.0) == 0.0);
    }

    SECTION("clamp above maximum")
    {
        REQUIRE(MathUtils::clamp(15.0, 0.0, 10.0) == 10.0);
    }
}

TEST_CASE("MathUtils lerp")
{
    SECTION("lerp at start")
    {
        REQUIRE(MathUtils::lerp(0.0, 10.0, 0.0) == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("lerp at end")
    {
        REQUIRE(MathUtils::lerp(0.0, 10.0, 1.0) == Approx(10.0).epsilon(EPSILON));
    }

    SECTION("lerp at middle")
    {
        REQUIRE(MathUtils::lerp(0.0, 10.0, 0.5) == Approx(5.0).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils inverseLerp")
{
    SECTION("inverseLerp at start")
    {
        REQUIRE(MathUtils::inverseLerp(0.0, 0.0, 10.0) == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("inverseLerp at end")
    {
        REQUIRE(MathUtils::inverseLerp(10.0, 0.0, 10.0) == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("inverseLerp at middle")
    {
        REQUIRE(MathUtils::inverseLerp(5.0, 0.0, 10.0) == Approx(0.5).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils mapLinear")
{
    SECTION("mapLinear identity")
    {
        REQUIRE(MathUtils::mapLinear(5.0, 0.0, 10.0, 0.0, 10.0) == Approx(5.0).epsilon(EPSILON));
    }

    SECTION("mapLinear double range")
    {
        REQUIRE(MathUtils::mapLinear(5.0, 0.0, 10.0, 0.0, 20.0) == Approx(10.0).epsilon(EPSILON));
    }

    SECTION("mapLinear half range")
    {
        REQUIRE(MathUtils::mapLinear(5.0, 0.0, 10.0, 0.0, 5.0) == Approx(2.5).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils angle conversion")
{
    SECTION("degToRad")
    {
        REQUIRE(MathUtils::degToRad(180.0) == Approx(3.14159265358979).epsilon(EPSILON));
        REQUIRE(MathUtils::degToRad(90.0) == Approx(1.5707963267949).epsilon(EPSILON));
        REQUIRE(MathUtils::degToRad(0.0) == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("radToDeg")
    {
        REQUIRE(MathUtils::radToDeg(3.14159265358979) == Approx(180.0).epsilon(EPSILON));
        REQUIRE(MathUtils::radToDeg(1.5707963267949) == Approx(90.0).epsilon(EPSILON));
        REQUIRE(MathUtils::radToDeg(0.0) == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("degToRad and radToDeg inverse")
    {
        REQUIRE(MathUtils::radToDeg(MathUtils::degToRad(45.0)) == Approx(45.0).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils smoothstep")
{
    SECTION("smoothstep boundaries")
    {
        REQUIRE(MathUtils::smoothstep(0.0, 0.0, 1.0) == Approx(0.0).epsilon(EPSILON));
        REQUIRE(MathUtils::smoothstep(1.0, 0.0, 1.0) == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("smoothstep middle")
    {
        REQUIRE(MathUtils::smoothstep(0.5, 0.0, 1.0) == Approx(0.5).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils smootherstep")
{
    SECTION("smootherstep boundaries")
    {
        REQUIRE(MathUtils::smootherstep(0.0, 0.0, 1.0) == Approx(0.0).epsilon(EPSILON));
        REQUIRE(MathUtils::smootherstep(1.0, 0.0, 1.0) == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils power of two")
{
    SECTION("isPowerOfTwo true")
    {
        REQUIRE(MathUtils::isPowerOfTwo(1) == true);
        REQUIRE(MathUtils::isPowerOfTwo(2) == true);
        REQUIRE(MathUtils::isPowerOfTwo(4) == true);
        REQUIRE(MathUtils::isPowerOfTwo(8) == true);
        REQUIRE(MathUtils::isPowerOfTwo(1024) == true);
    }

    SECTION("isPowerOfTwo false")
    {
        REQUIRE(MathUtils::isPowerOfTwo(0) == false);
        REQUIRE(MathUtils::isPowerOfTwo(3) == false);
        REQUIRE(MathUtils::isPowerOfTwo(5) == false);
        REQUIRE(MathUtils::isPowerOfTwo(7) == false);
    }

    SECTION("ceilPowerOfTwo")
    {
        REQUIRE(MathUtils::ceilPowerOfTwo(1) == 1);
        REQUIRE(MathUtils::ceilPowerOfTwo(2) == 2);
        REQUIRE(MathUtils::ceilPowerOfTwo(3) == 4);
        REQUIRE(MathUtils::ceilPowerOfTwo(5) == 8);
        REQUIRE(MathUtils::ceilPowerOfTwo(9) == 16);
    }

    SECTION("floorPowerOfTwo")
    {
        REQUIRE(MathUtils::floorPowerOfTwo(1) == 1);
        REQUIRE(MathUtils::floorPowerOfTwo(2) == 2);
        REQUIRE(MathUtils::floorPowerOfTwo(3) == 2);
        REQUIRE(MathUtils::floorPowerOfTwo(7) == 4);
        REQUIRE(MathUtils::floorPowerOfTwo(15) == 8);
    }
}

TEST_CASE("MathUtils euclideanModulo")
{
    SECTION("euclideanModulo positive")
    {
        REQUIRE(MathUtils::euclideanModulo(7, 5) == 2);
    }

    SECTION("euclideanModulo negative")
    {
        REQUIRE(MathUtils::euclideanModulo(-7, 5) == 3);
    }

    SECTION("euclideanModulo zero")
    {
        REQUIRE(MathUtils::euclideanModulo(10, 5) == 0);
    }
}

TEST_CASE("MathUtils pingpong")
{
    SECTION("pingpong within range")
    {
        REQUIRE(MathUtils::pingpong(2.0, 10.0) == Approx(2.0).epsilon(EPSILON));
    }

    SECTION("pingpong bounce")
    {
        REQUIRE(MathUtils::pingpong(12.0, 10.0) == Approx(8.0).epsilon(EPSILON));
    }

    SECTION("pingpong at boundaries")
    {
        REQUIRE(MathUtils::pingpong(0.0, 10.0) == Approx(0.0).epsilon(EPSILON));
        REQUIRE(MathUtils::pingpong(10.0, 10.0) == Approx(10.0).epsilon(EPSILON));
    }
}

TEST_CASE("MathUtils seededRandom")
{
    SECTION("seededRandom in range")
    {
        for (int i = 0; i < 100; i++)
        {
            float r = MathUtils::seededRandom();
            REQUIRE(r >= 0.0);
            REQUIRE(r < 1.0);
        }
    }
}

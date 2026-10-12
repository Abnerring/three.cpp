/**
 * @file   Quaternion.tests.cpp
 * @brief  Quaternion 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Quaternion.h"
#include "math/Vector3.h"
#include "math/Euler.h"
#include "core/BufferAttribute.h"
#include <utility>
#include <vector>

TEST_CASE("Quaternion property and bulk mutations follow upstream callback counts", "[properties]") {
    Quaternion q;
    int calls = 0;
    std::vector<std::vector<double>> observed;
    q._onChange([&] { ++calls; observed.push_back(q.toArray()); });
    q.x = .1; q.y = .2; q.z = .3; q.w = .4;
    REQUIRE(calls == 4);
    q.x = q.x;
    q.y *= 2;
    REQUIRE(calls == 6);
    q.set(1, 2, 3, 4);
    REQUIRE(calls == 7);
    REQUIRE(observed.back() == std::vector<double>{1, 2, 3, 4});
    q.setFromEuler(Euler(.2, .3, .4), false);
    REQUIRE(calls == 7);
    q.setFromEuler(Euler(.2, .3, .4));
    REQUIRE(calls == 8);
    q.slerp(Quaternion(), 0);
    REQUIRE(calls == 9);
    q.slerpQuaternions(Quaternion(), Quaternion(), .5);
    REQUIRE(calls == 11);
    q.fromArray(std::vector<double>{1, 2, 3, 4});
    REQUIRE(calls == 12);
    Float32BufferAttribute attr(std::vector<float>{5, 6, 7, 8}, 4);
    q.fromBufferAttribute(attr, 0);
    REQUIRE(calls == 13);
    REQUIRE(observed.back() == std::vector<double>{5, 6, 7, 8});
}

TEST_CASE("Quaternion snapshots copy values and assignments preserve target callbacks", "[properties]") {
    Quaternion source(1, 2, 3, 4), target;
    int sourceCalls = 0, targetCalls = 0, movedCalls = 0;
    source._onChange([&] { ++sourceCalls; });
    target._onChange([&] { ++targetCalls; });
    auto snapshot = source.x;
    source.x = 9;
    REQUIRE(double(snapshot) == 1);
    snapshot = 3;
    REQUIRE(double(source.x) == 9);
    REQUIRE(sourceCalls == 1);
    Quaternion copied(source), moved(std::move(source));
    copied.y = 10;
    moved._onChange([&] { ++movedCalls; });
    moved.z = 20;
    REQUIRE(sourceCalls == 1);
    REQUIRE(double(source.y) == 2);
    REQUIRE(double(source.z) == 3);
    target = copied;
    REQUIRE(targetCalls == 1);
    target.w = 30;
    REQUIRE(double(copied.w) == 4);
    target = std::move(moved);
    target.x = -4;
    REQUIRE(targetCalls == 4);
    REQUIRE(movedCalls == 1);
    REQUIRE(sourceCalls == 1);
    source.w = 5;
    REQUIRE(sourceCalls == 2);
}

TEST_CASE("Property assignment expressions return the value assigned before a callback", "[properties]") {
    Quaternion q;
    q._onChange([&] { q._onChange({}); q.set(9, 8, 7, 6); });
    const double result = (q.x = .5);
    REQUIRE(result == .5);
    REQUIRE(double(q.x) == 9);
}

TEST_CASE("Change callbacks retain mutable state and can replace themselves", "[properties]") {
    Quaternion q;
    std::vector<int> observed;
    q._onChange([count = 0, &observed]() mutable { observed.push_back(++count); });
    q.x = 1;
    q.set(2, 3, 4, 5);
    q.y = 6;
    REQUIRE(observed == std::vector<int>{1, 2, 3});
    int replacementCalls = 0;
    q._onChange([&] {
        q._onChange([&] { ++replacementCalls; });
        q.z = 7;
    });
    q.set(8, 9, 10, 11);
    REQUIRE(replacementCalls == 1);
    q.w = 12;
    REQUIRE(replacementCalls == 2);
}

#define EPSILON 1e-6

TEST_CASE("Quaternion Construction")
{
    SECTION("Default constructor")
    {
        Quaternion q;
        REQUIRE(q.x == 0.0);
        REQUIRE(q.y == 0.0);
        REQUIRE(q.z == 0.0);
        REQUIRE(q.w == 1.0);
    }

    SECTION("Parameterized constructor")
    {
        Quaternion q(1.0, 2.0, 3.0, 4.0);
        REQUIRE(q.x == 1.0);
        REQUIRE(q.y == 2.0);
        REQUIRE(q.z == 3.0);
        REQUIRE(q.w == 4.0);
    }
}

TEST_CASE("Quaternion setters")
{
    SECTION("set()")
    {
        Quaternion q;
        q.set(1.0, 2.0, 3.0, 4.0);
        REQUIRE(q.x == 1.0);
        REQUIRE(q.y == 2.0);
        REQUIRE(q.z == 3.0);
        REQUIRE(q.w == 4.0);
    }

    SECTION("setScalar()")
    {
        Quaternion q;
        q.setScalar(5.0);
        REQUIRE(q.x == 5.0);
        REQUIRE(q.y == 5.0);
        REQUIRE(q.z == 5.0);
        REQUIRE(q.w == 5.0);
    }

    SECTION("setX/setY/setZ/setW()")
    {
        Quaternion q;
        q.setX(1.0).setY(2.0).setZ(3.0).setW(4.0);
        REQUIRE(q.x == 1.0);
        REQUIRE(q.y == 2.0);
        REQUIRE(q.z == 3.0);
        REQUIRE(q.w == 4.0);
    }
}

TEST_CASE("Quaternion copy")
{
    SECTION("copy()")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b;
        b.copy(a);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
        REQUIRE(b.w == 4.0);
    }

    SECTION("clone()")
    {
        Quaternion a(1.0, 2.0, 3.0, 4.0);
        Quaternion b = a.clone();
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
    }
}

TEST_CASE("Quaternion normalize")
{
    SECTION("normalize non-unit")
    {
        Quaternion q(2.0, 0.0, 0.0, 0.0);
        q.normalize();
        REQUIRE(q.length() == Approx(1.0).epsilon(EPSILON));
        REQUIRE(q.x == Approx(1.0).epsilon(EPSILON));
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
        REQUIRE(q.x == -1.0);
        REQUIRE(q.y == -2.0);
        REQUIRE(q.z == -3.0);
        REQUIRE(q.w == 4.0);
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
        REQUIRE(q.w == 1.0);
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
        REQUIRE(result.w == Approx(1.0).epsilon(EPSILON));
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
        REQUIRE(result.x > 0.0);
        REQUIRE(result.w > 0.0);
    }
}

TEST_CASE("Quaternion scalar operations")
{
    SECTION("multiplyScalar()")
    {
        Quaternion q(1.0, 2.0, 3.0, 4.0);
        q.multiplyScalar(2.0);
        REQUIRE(q.x == 2.0);
        REQUIRE(q.y == 4.0);
        REQUIRE(q.z == 6.0);
        REQUIRE(q.w == 8.0);
    }

    SECTION("divideScalar()")
    {
        Quaternion q(2.0, 4.0, 6.0, 8.0);
        q.divideScalar(2.0);
        REQUIRE(q.x == 1.0);
        REQUIRE(q.y == 2.0);
        REQUIRE(q.z == 3.0);
        REQUIRE(q.w == 4.0);
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

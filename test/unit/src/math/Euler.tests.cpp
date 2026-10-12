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
#include <utility>
#include <vector>
#include <cmath>

TEST_CASE("Euler properties notify after each assignment and batch once", "[properties]") {
    Euler e;
    int calls = 0;
    std::vector<std::vector<double>> observed;
    e._onChange([&] { ++calls; observed.push_back({double(e.x), double(e.y), double(e.z)}); });
    e.x = .1; e.y = .2; e.z = .3; e.order = EulerOrder::YXZ;
    REQUIRE(calls == 4);
    e.x = e.x;
    REQUIRE(calls == 5);
    e.x += .4;
    REQUIRE(calls == 6);
    e.set(1, 2, 3);
    REQUIRE(calls == 7);
    REQUIRE(observed.back() == std::vector<double>{1, 2, 3});
    REQUIRE(e.order == EulerOrder::YXZ);
    e.setFromRotationMatrix(Matrix4(), std::nullopt, false);
    e.setFromQuaternion(Quaternion(), std::nullopt, false);
    REQUIRE(calls == 7);
    e.setFromRotationMatrix(Matrix4());
    REQUIRE(calls == 8);
    e.fromArray(std::vector<double>{4, 5, 6});
    REQUIRE(calls == 9);
    auto snapshot = e.x;
    e.x = 8;
    REQUIRE(double(snapshot) == 4);
    snapshot = -1;
    REQUIRE(double(e.x) == 8);
    REQUIRE(calls == 10);
}

TEST_CASE("Euler copied and moved properties bind to the destination", "[properties]") {
    Euler source(1, 2, 3), target;
    int sourceCalls = 0, targetCalls = 0, movedCalls = 0;
    source._onChange([&] { ++sourceCalls; });
    target._onChange([&] { ++targetCalls; });
    Euler copied(source);
    copied.x = 10;
    REQUIRE(double(source.x) == 1);
    REQUIRE(sourceCalls == 0);
    Euler moved(std::move(source));
    moved._onChange([&] { ++movedCalls; });
    moved.order = EulerOrder::ZYX;
    REQUIRE(movedCalls == 1);
    REQUIRE(source.order == EulerOrder::XYZ);
    target = copied;
    REQUIRE(targetCalls == 1);
    target.z = 12;
    REQUIRE(targetCalls == 2);
    REQUIRE(double(copied.z) == 3);
    target = std::move(moved);
    REQUIRE(targetCalls == 3);
    target.x = 20;
    REQUIRE(targetCalls == 4);
    REQUIRE(movedCalls == 1);
    REQUIRE(sourceCalls == 0);
    source.y = 9;
    REQUIRE(sourceCalls == 1);
}

TEST_CASE("Property callbacks synchronize rotation without recursion or copy leakage", "[properties]") {
    struct RotationOwner {
        Euler rotation;
        Quaternion quaternion;
        int rotationChanges = 0, quaternionChanges = 0;
        RotationOwner() {
            rotation._onChange([this] {
                ++rotationChanges;
                quaternion.setFromEuler(rotation, false);
            });
            quaternion._onChange([this] {
                ++quaternionChanges;
                rotation.setFromQuaternion(quaternion, std::nullopt, false);
            });
        }
    } owner;
    owner.rotation.x = .2;
    REQUIRE(owner.rotationChanges == 1);
    REQUIRE(owner.quaternionChanges == 0);
    REQUIRE(double(owner.quaternion.x) == Approx(std::sin(.1)));
    owner.quaternion.setFromAxisAngle(Vector3(0, 1, 0), .6);
    REQUIRE(owner.quaternionChanges == 1);
    REQUIRE(owner.rotationChanges == 1);
    REQUIRE(double(owner.rotation.y) == Approx(.6));
    auto rotationCopy = owner.rotation;
    auto quaternionCopy = owner.quaternion;
    rotationCopy.x = 4;
    quaternionCopy.w = .5;
    REQUIRE(owner.rotationChanges == 1);
    REQUIRE(owner.quaternionChanges == 1);
    owner.rotation = rotationCopy;
    REQUIRE(owner.rotationChanges == 2);
    owner.quaternion = std::move(quaternionCopy);
    REQUIRE(owner.quaternionChanges == 2);
    REQUIRE(owner.rotationChanges == 2);
}
#include "math/MathUtils.h"

#define EPSILON 1e-6

TEST_CASE("Euler Construction")
{
    SECTION("Default constructor")
    {
        Euler e;
        REQUIRE(e.x == 0.0);
        REQUIRE(e.y == 0.0);
        REQUIRE(e.z == 0.0);
        REQUIRE(e.order == EulerOrder::XYZ);
    }

    SECTION("Parameterized constructor")
    {
        Euler e(1.0, 2.0, 3.0, EulerOrder::YXZ);
        REQUIRE(e.x == 1.0);
        REQUIRE(e.y == 2.0);
        REQUIRE(e.z == 3.0);
        REQUIRE(e.order == EulerOrder::YXZ);
    }
}

TEST_CASE("Euler setters")
{
    SECTION("set()")
    {
        Euler e;
        e.set(1.0, 2.0, 3.0, EulerOrder::ZYX);
        REQUIRE(e.x == 1.0);
        REQUIRE(e.y == 2.0);
        REQUIRE(e.z == 3.0);
        REQUIRE(e.order == EulerOrder::ZYX);
    }

    SECTION("setX/setY/setZ()")
    {
        Euler e;
        e.setX(1.0).setY(2.0).setZ(3.0);
        REQUIRE(e.x == 1.0);
        REQUIRE(e.y == 2.0);
        REQUIRE(e.z == 3.0);
    }

    SECTION("setOrder()")
    {
        Euler e;
        e.setOrder(EulerOrder::YZX);
        REQUIRE(e.order == EulerOrder::YZX);
    }
}

TEST_CASE("Euler copy")
{
    SECTION("copy()")
    {
        Euler a(1.0, 2.0, 3.0, EulerOrder::XZY);
        Euler b;
        b.copy(a);
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
        REQUIRE(b.order == EulerOrder::XZY);
    }

    SECTION("clone()")
    {
        Euler a(1.0, 2.0, 3.0, EulerOrder::ZXY);
        Euler b = a.clone();
        REQUIRE(b.x == 1.0);
        REQUIRE(b.y == 2.0);
        REQUIRE(b.z == 3.0);
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
        REQUIRE(e.x == Approx(0.0).epsilon(EPSILON));
        REQUIRE(e.y == Approx(MathUtils::PI / 2).epsilon(EPSILON));
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

        REQUIRE(e1.order == EulerOrder::XYZ);
        REQUIRE(e2.order == EulerOrder::XZY);
        REQUIRE(e3.order == EulerOrder::YXZ);
        REQUIRE(e4.order == EulerOrder::YZX);
        REQUIRE(e5.order == EulerOrder::ZXY);
        REQUIRE(e6.order == EulerOrder::ZYX);
    }
}

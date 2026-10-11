#include "ThreeJsFixtures.h"
#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include "math/Color.h"
#include "math/Euler.h"
#include "math/MathUtils.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
void reference(const std::vector<double>& actual, const std::string& name) {
    const auto& expected = ThreeJsFixtures::data.at(name);
    REQUIRE(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        INFO(name << " component " << i);
        REQUIRE(actual[i] == Approx(expected[i]).epsilon(5e-13).margin(5e-13));
    }
}
} // namespace

TEST_CASE("Vector operations agree with upstream JavaScript", "[threejs-parity]") {
    static_assert(std::is_same_v<HIGH_PRECISION, double>);
    reference(Vector2(1.25, -2.5).applyMatrix3(Matrix3(2, 3, 5, 7, 11, 13, 17, 19, 23)).toArray(),
              "vector2Matrix");
    reference(Vector2(3, 4).normalize().toArray(), "vector2Normal");
    reference(Vector2(3, 4).clampLength(2, 3).toArray(), "vector2Clamp");
    reference(Vector3(1.25, -2.5, -8)
                  .applyMatrix4(Matrix4().makePerspective(-2, 3, 4, -1, 0.5, 25, 2001))
                  .toArray(),
              "vector3Projective");
    reference(Vector3(1, 2, 3).transformDirection(Matrix4().makeScale(2, 3, 4)).toArray(),
              "vector3Direction");
    reference(Vector3(1.25, -2.5, 3.75).applyQuaternion(Quaternion(0.2, 0.3, -0.4, 0.8)).toArray(),
              "vector3Quaternion");
    REQUIRE(Vector3().setFromEuler(Euler(0.3, -0.7, 1.2)).equals(Vector3(0.3, -0.7, 1.2)));
    Vector3 chained;
    REQUIRE(&chained.set(1, 2, 3).normalize().multiplyScalar(2).negate() == &chained);
}

TEST_CASE("All rotation orders agree with upstream JavaScript", "[threejs-parity]") {
    for (const auto order : {EulerOrder::XYZ, EulerOrder::XZY, EulerOrder::YXZ, EulerOrder::YZX,
                             EulerOrder::ZXY, EulerOrder::ZYX}) {
        const Euler e(0.3, -0.7, 1.2, order);
        const auto name = e.getOrderString();
        const auto m = Matrix4().makeRotationFromEuler(e);
        reference(m.toArray(), "rotation" + name);
        reference(Quaternion().setFromEuler(e).toArray(), "quaternion" + name);
        Euler result(0, 0, 0, order);
        result.setFromRotationMatrix(m);
        std::vector<double> values;
        result.toArray(values);
        reference(values, "euler" + name);
        result.setFromRotationMatrix(
            Matrix4().makeRotationFromEuler(Euler(MATH_PI / 2, MATH_PI / 2, MATH_PI / 2, order)));
        result.toArray(values);
        reference(values, "gimbal" + name);
        REQUIRE(result.order() == order);
        result.set(0.1, 0.2, 0.3);
        REQUIRE(result.order() == order);
    }
}

TEST_CASE("Quaternion edge cases agree with upstream JavaScript", "[threejs-parity]") {
    reference(Quaternion().setFromAxisAngle(Vector3(2, 0, 0), 0.7).toArray(), "axisNonunit");
    reference(Quaternion().setFromUnitVectors(Vector3(1, 0, 0), Vector3(0, 1, 0)).toArray(),
              "unitVectors");
    reference(Quaternion().setFromUnitVectors(Vector3(0, 0, 1), Vector3(0, 0, -1)).toArray(),
              "oppositeVectors");
    const auto a = Quaternion().setFromEuler(Euler(0.2, 0.3, 0.4));
    const auto b = Quaternion().setFromEuler(Euler(-0.8, 1.1, -0.7));
    reference(a.clone().slerp(Quaternion(-b.x(), -b.y(), -b.z(), -b.w()), 0.35).toArray(),
              "slerpNegative");
    reference(a.clone().slerp(b, -0.75).toArray(), "slerpExtrapolate");
    reference(
        Quaternion().slerp(Quaternion().setFromAxisAngle(Vector3(0, 1, 0), 0.0002), 0.2).toArray(),
        "slerpSmall");
    reference(Quaternion(0, 0, 0, 0).normalize().toArray(), "quaternionZero");
    reference(Quaternion(1, 2, 3, 4).invert().toArray(), "quaternionInverse");
    reference(Vector4().setAxisAngleFromQuaternion(a).toArray(), "axisQuaternion");
    reference(
        Vector4().setAxisAngleFromRotationMatrix(Matrix4().makeRotationFromQuaternion(a)).toArray(),
        "axisMatrix");
    reference(Vector4().setAxisAngleFromRotationMatrix(Matrix4()).toArray(), "axisIdentity");
    reference(Vector4().setAxisAngleFromRotationMatrix(Matrix4().makeRotationY(MATH_PI)).toArray(),
              "axisHalfTurn");
    auto src0 = a.toArray();
    src0.insert(src0.begin(), 90);
    src0.push_back(91);
    auto src1 = Quaternion(-b.x(), -b.y(), -b.z(), -b.w()).toArray();
    src1.insert(src1.begin(), {80, 81});
    std::vector<double> flat{70};
    Quaternion::slerpFlat(flat, 1, src0, 1, src1, 2, 0.35);
    reference(flat, "flatSlerp");
    flat = {70};
    Quaternion::multiplyQuaternionsFlat(flat, 1, src0, 1, src1, 2);
    reference(flat, "flatMultiply");
}

TEST_CASE("Projection and decomposition agree with upstream JavaScript", "[threejs-parity]") {
    for (const auto coordinate : {2000, 2001})
        for (const bool reversed : {false, true}) {
            const auto suffix = std::to_string(coordinate) + std::to_string(reversed);
            reference(
                Matrix4().makePerspective(-2, 3, 4, -1, 0.5, 25, coordinate, reversed).toArray(),
                "perspective" + suffix);
            reference(
                Matrix4().makeOrthographic(-2, 3, 4, -1, 0.5, 25, coordinate, reversed).toArray(),
                "orthographic" + suffix);
        }
    auto composed = Matrix4().compose(
        Vector3(5, -2, 7), Quaternion().setFromEuler(Euler(0.2, 0.3, 0.4)), Vector3(-2, 3, 4));
    Vector3 position, scale;
    Quaternion quaternion;
    composed.decompose(position, quaternion, scale);
    auto values = position.toArray();
    quaternion.toArray(values, 3);
    scale.toArray(values, 7);
    reference(values, "decompose");
    reference(Matrix3().getNormalMatrix(composed).toArray(), "normalMatrix");
    auto singular = Matrix4().makeScale(0, 2, 3).setPosition(5, -2, 7);
    singular.decompose(position, quaternion, scale);
    values = position.toArray();
    quaternion.toArray(values, 3);
    scale.toArray(values, 7);
    reference(values, "decomposeSingular");
    reference(Matrix4().extractRotation(singular).toArray(), "extractSingular");
    REQUIRE(Matrix3(0, 0, 0, 0, 0, 0, 0, 0, 0).invert().equals(Matrix3(0, 0, 0, 0, 0, 0, 0, 0, 0)));
    REQUIRE_THROWS_AS(Matrix4().makePerspective(-1, 1, 1, -1, 1, 10, 123), std::invalid_argument);
}

TEST_CASE("Color and MathUtils agree with upstream JavaScript", "[threejs-parity]") {
    reference(Color("#123abc").toArray(), "colorhex");
    reference(Color("#f82").toArray(), "colorshort");
    reference(Color("rgb(125, 64, 255)").toArray(), "colorrgb");
    reference(Color("rgb(50.5%, 0%, 100%)").toArray(), "colorpercent");
    reference(Color("hsl(-120, 60%, 40%)").toArray(), "colorhsl");
    reference(Color("rebeccapurple").toArray(), "colorname");
    reference(Color(0.1, 0.2, 0.3).sub(Color(0.2, 0.1, 0.8)).toArray(), "colorSub");
    reference(Color().setHSL(2.2, 1.7, 0.4).toArray(), "colorHsl");
    reference(Color().setHSL(0.9, 0.5, 0.6).lerpHSL(Color().setHSL(0.1, 0.7, 0.2), 0.3).toArray(),
              "colorLerpHsl");
    reference(Color().setHSL(0.2, 0.4, 0.6).offsetHSL(0.3, 0.2, -0.1).toArray(), "colorOffsetHsl");
    REQUIRE(Color("#123abc").getHexString() == "123abc");
    REQUIRE(Color("#123abc").getStyle() == "rgb(18,58,188)");
    REQUIRE(Color(0.1, 0.2, 0.3).setColorName("unknown").equals(Color(0.1, 0.2, 0.3)));
    std::vector<double> seeded{MathUtils::seededRandom(123)};
    for (size_t i = 1; i < 8; ++i)
        seeded.push_back(MathUtils::seededRandom());
    reference(seeded, "seededRandom");
    reference({MathUtils::euclideanModulo(-1.25, 0.75), MathUtils::pingpong(2.75, 1.5),
               MathUtils::inverseLerp(-2, 5, 0.7), MathUtils::degToRad(37.2),
               MathUtils::radToDeg(0.4)},
              "mathUtils");
    std::vector<double> powers;
    for (const double value :
         {4294967297.0, 4294967296.0, 1.5, 9007199254740992.0, -2.0, 0.0,
          std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
          -std::numeric_limits<double>::infinity()})
        powers.push_back(MathUtils::isPowerOfTwo(value));
    reference(powers, "powerOfTwo");
    for (const std::string order : {"XYX", "XZX", "YXY", "YZY", "ZXZ", "ZYZ"}) {
        Quaternion quaternion;
        MathUtils::setQuaternionFromProperEuler(quaternion, 0.3, -0.7, 1.2, order);
        reference(quaternion.toArray(), "proper" + order);
    }
    reference(Color(0.1, 0.2, 0.3).applyMatrix3(Matrix3(2, 3, 5, 7, 11, 13, 17, 19, 23)).toArray(),
              "colorMatrix");
    REQUIRE(MathUtils::denormalize<float>(0.4) == 0.4);
    REQUIRE(MathUtils::normalize<int8_t>(-0.5) == -63);
    REQUIRE(std::isnan(MathUtils::euclideanModulo(1, 0)));
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    reference({double(std::isnan(MathUtils::min(nan, 1))),
               double(std::isnan(MathUtils::max(1, nan))),
               double(std::signbit(MathUtils::min(0, -0.0))),
               double(std::signbit(MathUtils::max(-0.0, 0)))},
              "minMaxEdges");
    REQUIRE(std::isnan(Vector3(1, 2, 3).min(Vector3(nan, 0, 0)).x()));
}

TEST_CASE("Determinant preserves upstream floating-point evaluation order", "[threejs-parity]") {
    const Matrix4 cancellation(80000, -100000, 90000, -70000, -100000, 50000, -70000, 40000, -70000,
                               90000, -80000, 80000, -20000, -50000, 20000, -29999.99999);
    REQUIRE(cancellation.determinant() == ThreeJsFixtures::data.at("determinantCancellation")[0]);
    const Matrix4 overflow(1e200, 0, 0, 0, 0, 1e200, 0, 0, 0, 0, 1e-200, 0, 0, 0, 0, 1e-200);
    REQUIRE(overflow.determinant() == ThreeJsFixtures::data.at("determinantOverflow")[0]);
    const Matrix4 underflow(1e-200, 0, 0, 0, 0, 1e-200, 0, 0, 0, 0, 1e200, 0, 0, 0, 0, 1e200);
    REQUIRE(underflow.determinant() == ThreeJsFixtures::data.at("determinantUnderflow")[0]);
}

TEST_CASE("Camera projection vectors agree with upstream JavaScript", "[threejs-parity]") {
    PerspectiveCamera camera(55, 1.5, 0.5, 25);
    camera.coordinateSystem = 2001;
    camera.position.set(5, -2, 7);
    camera.rotation.set(0.2, 0.3, 0.4);
    camera.updateProjectionMatrix();
    camera.updateMatrixWorld(true);
    reference(Vector3(1.2, -0.5, -2.3).project(camera).toArray(), "cameraProject");
    reference(Vector3(0.3, -0.6, 0.7).unproject(camera).toArray(), "cameraUnproject");
}

TEST_CASE("Mutation callbacks and strict equality follow JavaScript", "[threejs-parity]") {
    int ec = 0, qc = 0;
    Euler e;
    Quaternion q;
    e._onChange([&] { ++ec; });
    q._onChange([&] { ++qc; });
    const auto a = Quaternion().setFromEuler(Euler(0.2, 0.3, 0.4));
    const auto b = Quaternion().setFromEuler(Euler(-0.8, 1.1, -0.7));
    e.setX(0.3);
    e.set(0.2, 0.3, 0.4);
    e.setFromQuaternion(a, EulerOrder::YXZ, false);
    e.setFromQuaternion(a);
    q.setX(0.3);
    q.copy(a);
    q.setFromEuler(e, false);
    q.setFromEuler(e);
    q.slerp(b, 0);
    q.normalize();
    reference({double(ec), double(qc)}, "callbacks");
    const int before = qc;
    Quaternion copied = q;
    copied.invert();
    REQUIRE(qc == before);
    q = copied;
    REQUIRE(qc == before + 1);
    REQUIRE_FALSE(Vector3(1, 2, 3).equals(Vector3(1 + 1e-9, 2, 3)));
    REQUIRE_FALSE(Quaternion().equals(Quaternion(0, 0, 0, 1 + 1e-9)));
    REQUIRE_FALSE(Color(0.1, 0.2, 0.3).equals(Color(0.1 + 1e-9, 0.2, 0.3)));
    const auto infinity = std::numeric_limits<double>::infinity();
    REQUIRE(Vector3(infinity, 0, 0).equals(Vector3(infinity, 0, 0)));
    const auto rounded = Vector4(-0.5, -1.5, 0.5, 1.5).round();
    REQUIRE(rounded.x() == 0);
    REQUIRE(std::signbit(rounded.x()));
    REQUIRE(rounded.y() == -1);
    REQUIRE(rounded.z() == 1);
    REQUIRE(rounded.w() == 2);
    const auto array = e.toArray();
    REQUIRE(std::get<std::string>(array[3]) == e.getOrderString());
}

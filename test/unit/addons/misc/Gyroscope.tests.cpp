#include "misc/Gyroscope.h"

#include "math/Euler.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#include "catch.hpp"

#include <cmath>
#include <memory>

namespace {
void requireMatricesMatch(const Matrix4& actual, const Matrix4& expected) {
    for (std::size_t i = 0; i < 16; ++i) {
        REQUIRE(std::abs(actual.elements()[i] - expected.elements()[i]) < 1e-12);
    }
}
}  // namespace

TEST_CASE("Gyroscope is an Object3D with the default matrix flags") {
    Gyroscope gyroscope;
    REQUIRE(gyroscope.type == "Object3D");
    REQUIRE(gyroscope.matrixAutoUpdate);
    REQUIRE(gyroscope.matrixWorldNeedsUpdate == false);
}

TEST_CASE("Gyroscope without a parent copies its local matrix") {
    Gyroscope gyroscope;
    gyroscope.position.set(1, 2, 3);
    gyroscope.quaternion.setFromEuler(Euler(0.3, 0.5, 0.7));
    gyroscope.scale.set(2, 3, 4);
    gyroscope.updateMatrixWorld();
    requireMatricesMatch(gyroscope.matrixWorld, gyroscope.matrix);
}

TEST_CASE("Gyroscope keeps its local rotation but the parent world translation") {
    auto parent = std::make_shared<Object3D>();
    parent->position.set(10, 20, 30);
    parent->quaternion.setFromEuler(Euler(0.1, 0.2, 0.3));
    auto gyroscope = std::make_shared<Gyroscope>();
    gyroscope->position.set(1, 2, 3);
    gyroscope->quaternion.setFromEuler(Euler(0.4, 0.5, 0.6));
    parent->add(gyroscope);
    parent->updateMatrixWorld(true);

    // Recompose the expected matrix independently from the multiplication result.
    Matrix4 combined;
    combined.multiplyMatrices(parent->matrixWorld, gyroscope->matrix);
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
    combined.decompose(translation, rotation, scale);
    Vector3 localTranslation;
    Quaternion localRotation;
    Vector3 localScale;
    gyroscope->matrix.decompose(localTranslation, localRotation, localScale);
    Matrix4 expected;
    expected.compose(translation, localRotation, scale);
    requireMatricesMatch(gyroscope->matrixWorld, expected);
}

TEST_CASE("Gyroscope propagates the forced update to its children") {
    auto gyroscope = std::make_shared<Gyroscope>();
    gyroscope->position.set(0, 5, 0);
    auto child = std::make_shared<Object3D>();
    child->position.set(0, 0, 2);
    gyroscope->add(child);
    gyroscope->updateMatrixWorld(true);

    REQUIRE(child->matrixWorldNeedsUpdate == false);
    const auto elements = child->matrixWorld.elements();
    // The child sits two units along its parent's local z axis, with the parent translated by y = 5.
    REQUIRE(std::abs(elements[12] - 0.0) < 1e-12);
    REQUIRE(std::abs(elements[13] - 5.0) < 1e-12);
    REQUIRE(std::abs(elements[14] - 2.0) < 1e-12);
}

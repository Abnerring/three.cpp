#include "catch.hpp"
#include "core/Object3D.h"
#include <memory>

// A focused check of the pieces the object loader's matrix path depends on: the default of matrixAutoUpdate,
// reading a matrix out of an array, and decomposing it.
//
// The array is COLUMN-major, which is the convention three uses everywhere - Matrix4.toArray writes the
// storage order and Matrix4.fromArray reads it back. A translation of (4, 5, 6) therefore lives at indices
// 12, 13 and 14, not at 3, 7 and 11. Getting this wrong once made a correct implementation look broken.
TEST_CASE("Object3D reads a column-major matrix through decompose") {
    auto object = std::make_shared<Object3D>();
    REQUIRE(object->matrixAutoUpdate);       // upstream defaults this to true
    REQUIRE(object->matrixWorldAutoUpdate);
    object->matrix.fromArray({1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 4.0, 5.0, 6.0, 1.0});
    REQUIRE(object->matrix.elements()[12] == 4.0);
    REQUIRE(object->matrix.elements()[13] == 5.0);
    REQUIRE(object->matrix.elements()[14] == 6.0);
    object->matrix.decompose(object->position, object->quaternion, object->scale);
    REQUIRE(object->position.x == 4.0);
    REQUIRE(object->position.y == 5.0);
    REQUIRE(object->position.z == 6.0);
    REQUIRE(object->scale.x == 1.0);
    REQUIRE(object->quaternion.w == 1.0);
}
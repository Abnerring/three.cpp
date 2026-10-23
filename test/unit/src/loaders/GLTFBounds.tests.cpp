#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <array>
#include <string>

TEST_CASE("GLTFLoader builds bounds from a position accessor") {
    // Both corners given: the box is what the file says.
    const auto box = GLTFLoader::boundsFromAccessor(true, true, {-1, -2, -3}, {1, 2, 3}, false,
                                                   "float32");
    REQUIRE(box.present);
    REQUIRE(box.bounds.min[0] == -1);
    REQUIRE(box.bounds.max[2] == 3);
}

TEST_CASE("GLTFLoader refuses a position accessor without corners") {
    // Either corner missing means no box, which is what some extensions of the format do.
    REQUIRE_FALSE(GLTFLoader::boundsFromAccessor(false, true, {0, 0, 0}, {1, 1, 1}, false, "float32")
                      .present);
    REQUIRE_FALSE(GLTFLoader::boundsFromAccessor(true, false, {0, 0, 0}, {1, 1, 1}, false, "float32")
                      .present);
    REQUIRE_FALSE(GLTFLoader::boundsFromAccessor(false, false, {0, 0, 0}, {1, 1, 1}, false, "float32")
                      .present);
}

TEST_CASE("GLTFLoader scales the bounds of a normalised accessor") {
    // The stored integers are scaled by the same factor at both corners.
    const auto box = GLTFLoader::boundsFromAccessor(true, true, {-127, -127, -127}, {127, 127, 127},
                                                   true, "int8");
    REQUIRE(box.present);
    REQUIRE(box.bounds.max[0] == Approx(1.0).epsilon(1e-12));
    REQUIRE(box.bounds.min[0] == Approx(-1.0).epsilon(1e-12));
    // An unsigned type scales differently, so the factor is not a constant.
    const auto unsigned_ = GLTFLoader::boundsFromAccessor(true, true, {0, 0, 0}, {255, 255, 255},
                                                         true, "uint8");
    REQUIRE(unsigned_.bounds.max[0] == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("GLTFLoader warns about a position without corners") {
    REQUIRE(GLTFLoader::boundsWarning() ==
            "THREE.GLTFLoader: Missing min/max properties for accessor POSITION.");
}

TEST_CASE("GLTFLoader measures a morph target by magnitude") {
    // The larger magnitude of the two corners, not the distance between them.
    const auto reach = GLTFLoader::morphDisplacement({-3, -1, 2}, {1, 5, -4});
    REQUIRE(reach[0] == 3);
    REQUIRE(reach[1] == 5);
    REQUIRE(reach[2] == 4);
    // A corner pair spanning the same distance the other way round gives a different answer,
    // which is what shows the magnitude is what matters.
    const auto offset = GLTFLoader::morphDisplacement({10, 0, 0}, {11, 0, 0});
    REQUIRE(offset[0] == 11);
}
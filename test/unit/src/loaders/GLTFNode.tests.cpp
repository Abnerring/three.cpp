#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <array>

namespace {
const std::array<double, 16> kIdentity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}  // namespace

TEST_CASE("GLTFLoader takes a node matrix over the three") {
    // A matrix of its own, with the three also given: the matrix wins.
    std::array<double, 16> matrix = kIdentity;
    matrix[12] = 5;
    const auto transform = GLTFLoader::nodeTransform(
        true, matrix, true, {9, 9, 9}, true, {1, 1, 1, 1}, true, {2, 2, 2});
    REQUIRE(transform.hasMatrix);
    REQUIRE(transform.matrix[12] == 5);
    // The three keep the values a fresh object has.
    REQUIRE(transform.translation[0] == 0);
    REQUIRE(transform.rotation[3] == 1);
    REQUIRE(transform.scale[0] == 1);
}

TEST_CASE("GLTFLoader takes each of the three on its own") {
    // Only a translation.
    const auto moved = GLTFLoader::nodeTransform(false, kIdentity, true, {1, 2, 3}, false, {},
                                                 false, {});
    REQUIRE_FALSE(moved.hasMatrix);
    REQUIRE(moved.translation[0] == 1);
    REQUIRE(moved.translation[2] == 3);
    // The other two are untouched.
    REQUIRE(moved.rotation[3] == 1);
    REQUIRE(moved.scale[0] == 1);

    // Only a rotation.
    const auto turned = GLTFLoader::nodeTransform(false, kIdentity, false, {}, true, {0, 0, 0, 1},
                                                  false, {});
    REQUIRE(turned.rotation[3] == 1);
    REQUIRE(turned.translation[0] == 0);
}

TEST_CASE("GLTFLoader takes all three") {
    const auto transform = GLTFLoader::nodeTransform(false, kIdentity, true, {1, 2, 3}, true,
                                                     {0, 0, 0, 1}, true, {4, 5, 6});
    REQUIRE(transform.translation[1] == 2);
    REQUIRE(transform.rotation[3] == 1);
    REQUIRE(transform.scale[0] == 4);
    REQUIRE(transform.scale[2] == 6);
}

TEST_CASE("GLTFLoader leaves an empty node at rest") {
    // A node that declares nothing sits at the origin, facing forward, at unit scale.
    const auto transform = GLTFLoader::nodeTransform(false, kIdentity, false, {}, false, {},
                                                     false, {});
    REQUIRE_FALSE(transform.hasMatrix);
    REQUIRE(transform.translation[0] == 0);
    REQUIRE(transform.translation[1] == 0);
    REQUIRE(transform.translation[2] == 0);
    REQUIRE(transform.rotation[0] == 0);
    REQUIRE(transform.rotation[3] == 1);
    REQUIRE(transform.scale[0] == 1);
    REQUIRE(transform.scale[1] == 1);
    REQUIRE(transform.scale[2] == 1);
    // The matrix field holds the identity even though it is unused.
    REQUIRE(transform.matrix[0] == 1);
    REQUIRE(transform.matrix[5] == 1);
    REQUIRE(transform.matrix[15] == 1);
}
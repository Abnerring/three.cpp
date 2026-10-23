#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("GLTFLoader writes a sparse override") {
    // Two elements of three components, all zero to begin with.
    std::vector<double> target{0, 0, 0, 0, 0, 0};
    // The second element is replaced by seven, eight and nine.
    GLTFLoader::applySparse({1}, {7, 8, 9}, 3, target);
    REQUIRE(target[0] == 0);
    REQUIRE(target[3] == 7);
    REQUIRE(target[4] == 8);
    REQUIRE(target[5] == 9);
}

TEST_CASE("GLTFLoader writes several sparse entries") {
    std::vector<double> target{0, 0, 0, 0, 0, 0};
    // Two entries, the second and the first, in that order.
    GLTFLoader::applySparse({1, 0}, {1, 2, 3, 4, 5, 6}, 3, target);
    REQUIRE(target[0] == 4);
    REQUIRE(target[1] == 5);
    REQUIRE(target[2] == 6);
    REQUIRE(target[3] == 1);
    REQUIRE(target[4] == 2);
    REQUIRE(target[5] == 3);
}

TEST_CASE("GLTFLoader ignores a sparse index past the end") {
    std::vector<double> target{1, 2, 3};
    // Index one is past the only element, so nothing changes.
    GLTFLoader::applySparse({1}, {9, 9, 9}, 3, target);
    REQUIRE(target[0] == 1);
    REQUIRE(target[2] == 3);
}

TEST_CASE("GLTFLoader leaves the rest when the values stop short") {
    std::vector<double> target{0, 0, 0};
    // Two values cannot fill an element of three, so the third keeps its old value.
    GLTFLoader::applySparse({0}, {7, 8}, 3, target);
    REQUIRE(target[0] == 7);
    REQUIRE(target[1] == 8);
    REQUIRE(target[2] == 0);
}

TEST_CASE("GLTFLoader writes nothing for an empty shape") {
    std::vector<double> target{1, 2, 3};
    // An item size of zero has no components to write.
    GLTFLoader::applySparse({0}, {9}, 0, target);
    REQUIRE(target[0] == 1);
    REQUIRE(target[2] == 3);
    // No entries at all leaves it alone too.
    std::vector<double> other{1, 2, 3};
    GLTFLoader::applySparse({}, {}, 1, other);
    REQUIRE(other[0] == 1);
}

TEST_CASE("GLTFLoader never reports a sparse accessor as normalised") {
    // Whatever was declared, the override turns the flag off.
    REQUIRE_FALSE(GLTFLoader::sparseNormalized(true));
    REQUIRE_FALSE(GLTFLoader::sparseNormalized(false));
}
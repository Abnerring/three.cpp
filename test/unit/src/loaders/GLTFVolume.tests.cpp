#include "loaders/GLTFLoader.h"

#include "catch.hpp"

TEST_CASE("GLTFLoader reads an attenuation distance") {
    // A distance that was written is taken as written.
    const auto given = GLTFLoader::attenuationDistance(true, 2.5);
    REQUIRE_FALSE(given.infinite);
    REQUIRE(given.distance == 2.5);
    // One that was not written means no limit.
    const auto absent = GLTFLoader::attenuationDistance(false, 2.5);
    REQUIRE(absent.infinite);
}

TEST_CASE("GLTFLoader reads a zero attenuation distance as no limit") {
    // The source writes `attenuationDistance || Infinity`, and zero is falsy there, so a
    // distance of zero cannot be told apart from one that was left out.
    const auto zero = GLTFLoader::attenuationDistance(true, 0.0);
    REQUIRE(zero.infinite);
    // A distance of zero and one that was omitted agree.
    const auto absent = GLTFLoader::attenuationDistance(false, 0.0);
    REQUIRE(zero.infinite == absent.infinite);
    // A very small distance is still a distance.
    const auto tiny = GLTFLoader::attenuationDistance(true, 1e-9);
    REQUIRE_FALSE(tiny.infinite);
    REQUIRE(tiny.distance == 1e-9);
}

TEST_CASE("GLTFLoader reads a thickness") {
    // A thickness that was written is taken.
    REQUIRE(GLTFLoader::volumeThickness(true, 3.0) == 3.0);
    // One that was not reads as zero rather than as the material's own value.
    REQUIRE(GLTFLoader::volumeThickness(false, 3.0) == 0.0);
    // A written zero stays zero, which the attenuation distance does not.
    REQUIRE(GLTFLoader::volumeThickness(true, 0.0) == 0.0);
}
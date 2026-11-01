#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFLoader knows the three light types") {
    REQUIRE(GLTFLoader::isLightType("directional"));
    REQUIRE(GLTFLoader::isLightType("point"));
    REQUIRE(GLTFLoader::isLightType("spot"));
    // The case matters, and so does an empty name.
    REQUIRE_FALSE(GLTFLoader::isLightType("Directional"));
    REQUIRE_FALSE(GLTFLoader::isLightType(""));
    REQUIRE_FALSE(GLTFLoader::isLightType("area"));
}

TEST_CASE("GLTFLoader reads a light range") {
    // A range that was written is taken.
    REQUIRE(GLTFLoader::lightRange(true, 5.0) == 5.0);
    // One that was not reads as zero, which means no limit.
    REQUIRE(GLTFLoader::lightRange(false, 5.0) == 0.0);
    REQUIRE(GLTFLoader::lightRange(false, 0.0) == 0.0);
}

TEST_CASE("GLTFLoader reads a light colour") {
    // White when nothing was written.
    const auto white = GLTFLoader::lightColour(false, {});
    REQUIRE(white[0] == 1.0);
    REQUIRE(white[1] == 1.0);
    REQUIRE(white[2] == 1.0);
    // The three numbers as they stand, with no conversion.
    const auto tinted = GLTFLoader::lightColour(true, {0.5, 0.25, 0.0});
    REQUIRE(tinted[0] == 0.5);
    REQUIRE(tinted[1] == 0.25);
    REQUIRE(tinted[2] == 0.0);
    // A colour written with too few numbers falls back to white rather than reading past it.
    const auto short_ = GLTFLoader::lightColour(true, {0.5, 0.25});
    REQUIRE(short_[0] == 1.0);
}

TEST_CASE("GLTFLoader points a directional light at the negative z axis") {
    const auto target = GLTFLoader::directionalTarget();
    REQUIRE(target[0] == 0.0);
    REQUIRE(target[1] == 0.0);
    REQUIRE(target[2] == -1.0);
}
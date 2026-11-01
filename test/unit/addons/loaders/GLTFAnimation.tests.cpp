#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader renames an animation path") {
    // Only the scale keeps its name; the other three are renamed.
    REQUIRE(GLTFLoader::pathProperty("scale") == "scale");
    REQUIRE(GLTFLoader::pathProperty("translation") == "position");
    REQUIRE(GLTFLoader::pathProperty("rotation") == "quaternion");
    REQUIRE(GLTFLoader::pathProperty("weights") == "morphTargetInfluences");
    // A path the format does not allow is refused.
    REQUIRE_THROWS_AS(GLTFLoader::pathProperty("colour"), std::runtime_error);
    // The comparison is exact, so the spelling matters.
    REQUIRE_THROWS_AS(GLTFLoader::pathProperty("Translation"), std::runtime_error);
}

TEST_CASE("GLTFLoader checks an alpha mode") {
    // The three words the format allows.
    REQUIRE(GLTFLoader::isValidAlphaMode("OPAQUE"));
    REQUIRE(GLTFLoader::isValidAlphaMode("MASK"));
    REQUIRE(GLTFLoader::isValidAlphaMode("BLEND"));
    // The case matters, since the mode is stored as written.
    REQUIRE_FALSE(GLTFLoader::isValidAlphaMode("opaque"));
    REQUIRE_FALSE(GLTFLoader::isValidAlphaMode("blend"));
    // Anything else is not a mode.
    REQUIRE_FALSE(GLTFLoader::isValidAlphaMode(""));
    REQUIRE_FALSE(GLTFLoader::isValidAlphaMode("TRANSLUCENT"));
}

TEST_CASE("GLTFLoader maps an interpolation") {
    // The two that map onto the port's own interpolants.
    REQUIRE(GLTFLoader::interpolationName("LINEAR") == "InterpolateLinear");
    REQUIRE(GLTFLoader::interpolationName("STEP") == "InterpolateDiscrete");
    // A cubic spline has no interpolant of its own, so its name is passed through.
    REQUIRE(GLTFLoader::interpolationName("CUBICSPLINE") == "CUBICSPLINE");
    // Anything else is refused.
    REQUIRE_THROWS_AS(GLTFLoader::interpolationName("LINEARLY"), std::runtime_error);
    REQUIRE_THROWS_AS(GLTFLoader::interpolationName(""), std::runtime_error);
}
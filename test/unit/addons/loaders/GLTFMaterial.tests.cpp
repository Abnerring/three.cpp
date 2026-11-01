#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFLoader starts a material white") {
    const auto colour = GLTFLoader::materialDefaults();
    REQUIRE(colour.r == 1.0);
    REQUIRE(colour.g == 1.0);
    REQUIRE(colour.b == 1.0);
    REQUIRE(colour.a == 1.0);
}

TEST_CASE("GLTFLoader reads a base colour factor") {
    // Four numbers fill all four pieces.
    const auto colour =
        GLTFLoader::applyBaseColorFactor(GLTFLoader::materialDefaults(), {0.5, 0.25, 0.0, 0.75});
    REQUIRE(colour.r == 0.5);
    REQUIRE(colour.g == 0.25);
    REQUIRE(colour.b == 0.0);
    REQUIRE(colour.a == 0.75);
    // The numbers are taken as they stand, with no conversion.
    const auto raw =
        GLTFLoader::applyBaseColorFactor(GLTFLoader::materialDefaults(), {0.2, 0.4, 0.6, 1.0});
    REQUIRE(raw.r == 0.2);
    REQUIRE(raw.b == 0.6);
}

TEST_CASE("GLTFLoader leaves what a short factor omits") {
    // One number sets the red alone.
    const auto one = GLTFLoader::applyBaseColorFactor(GLTFLoader::materialDefaults(), {0.5});
    REQUIRE(one.r == 0.5);
    REQUIRE(one.g == 1.0);
    REQUIRE(one.b == 1.0);
    REQUIRE(one.a == 1.0);
    // Three set the colour but not the alpha.
    const auto three =
        GLTFLoader::applyBaseColorFactor(GLTFLoader::materialDefaults(), {0.1, 0.2, 0.3});
    REQUIRE(three.b == 0.3);
    REQUIRE(three.a == 1.0);
    // An empty factor changes nothing.
    const auto none = GLTFLoader::applyBaseColorFactor(GLTFLoader::materialDefaults(), {});
    REQUIRE(none.r == 1.0);
    REQUIRE(none.a == 1.0);
}

TEST_CASE("GLTFLoader keeps the two colour spaces apart") {
    // The factor is already linear while the texture is display referred.
    REQUIRE(std::string(GLTFLoader::kBaseColorFactorSpace) == "srgb-linear");
    REQUIRE(std::string(GLTFLoader::kBaseColorTextureSpace) == "srgb");
    REQUIRE(std::string(GLTFLoader::kBaseColorFactorSpace) !=
            std::string(GLTFLoader::kBaseColorTextureSpace));
}
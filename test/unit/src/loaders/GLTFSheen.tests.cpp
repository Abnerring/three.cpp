#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader starts a sheen black") {
    const auto sheen = GLTFLoader::sheenDefaults();
    // The colour starts black rather than white.
    REQUIRE(sheen.colour[0] == 0.0);
    REQUIRE(sheen.colour[1] == 0.0);
    REQUIRE(sheen.colour[2] == 0.0);
    // A smooth sheen, turned all the way up.
    REQUIRE(sheen.roughness == 0.0);
    REQUIRE(sheen.sheen == 1.0);
}

TEST_CASE("GLTFLoader starts a material and a sheen differently") {
    // The two colours default the opposite way round, which is easy to get wrong.
    const auto material = GLTFLoader::materialDefaults();
    const auto sheen = GLTFLoader::sheenDefaults();
    REQUIRE(material.r == 1.0);
    REQUIRE(sheen.colour[0] == 0.0);
    REQUIRE(material.r != sheen.colour[0]);
}

TEST_CASE("GLTFLoader renames a sheen field") {
    // Both fields are factors, so both lose their suffix.
    REQUIRE(GLTFLoader::sheenParamForField("sheenColorFactor") == "sheenColor");
    REQUIRE(GLTFLoader::sheenParamForField("sheenRoughnessFactor") == "sheenRoughness");
    // The extension has only those two.
    REQUIRE(GLTFLoader::sheenParamForField("sheenColorTexture").empty());
    REQUIRE(GLTFLoader::sheenParamForField("sheen").empty());
    REQUIRE(GLTFLoader::sheenParamForField("").empty());
}
#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader renames a clearcoat field") {
    // A factor drops its suffix and a texture gains a map suffix.
    REQUIRE(GLTFLoader::materialParamForField("clearcoatFactor") == "clearcoat");
    REQUIRE(GLTFLoader::materialParamForField("clearcoatTexture") == "clearcoatMap");
    // A roughness field keeps its name on the factor and gains a map on the texture.
    REQUIRE(GLTFLoader::materialParamForField("clearcoatRoughnessFactor") == "clearcoatRoughness");
    REQUIRE(GLTFLoader::materialParamForField("clearcoatRoughnessTexture") ==
            "clearcoatRoughnessMap");
    // The normal texture and its scale are two different parameters.
    REQUIRE(GLTFLoader::materialParamForField("clearcoatNormalTexture") == "clearcoatNormalMap");
    REQUIRE(GLTFLoader::materialParamForField("clearcoatNormalTexture.scale") ==
            "clearcoatNormalScale");
    // The two normal entries do not collide.
    REQUIRE(GLTFLoader::materialParamForField("clearcoatNormalTexture") !=
            GLTFLoader::materialParamForField("clearcoatNormalTexture.scale"));
}

TEST_CASE("GLTFLoader names no parameter for an unknown field") {
    REQUIRE(GLTFLoader::materialParamForField("clearcoat").empty());
    REQUIRE(GLTFLoader::materialParamForField("clearcoatColour").empty());
    REQUIRE(GLTFLoader::materialParamForField("").empty());
}

TEST_CASE("GLTFLoader picks the physical material for a clearcoat") {
    // Only the physical material carries a clearcoat.
    REQUIRE(GLTFLoader::materialTypeForClearcoat(true) == "MeshPhysicalMaterial");
    // A material without the extension names no type of its own.
    REQUIRE(GLTFLoader::materialTypeForClearcoat(false).empty());
}

TEST_CASE("GLTFLoader writes a normal scale on both axes") {
    // The one number the file wrote goes on both.
    const auto scale = GLTFLoader::normalScaleFrom(0.5);
    REQUIRE(scale[0] == 0.5);
    REQUIRE(scale[1] == 0.5);
    // A scale of one leaves the texture alone.
    const auto one = GLTFLoader::normalScaleFrom(1.0);
    REQUIRE(one[0] == 1.0);
    REQUIRE(one[1] == 1.0);
}
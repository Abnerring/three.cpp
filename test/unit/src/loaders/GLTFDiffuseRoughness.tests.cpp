#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader renames a diffuse roughness field") {
    // The factor drops its suffix and the texture gains a map suffix.
    REQUIRE(GLTFLoader::diffuseRoughnessParamForField("diffuseRoughnessFactor") ==
            "diffuseRoughness");
    REQUIRE(GLTFLoader::diffuseRoughnessParamForField("diffuseRoughnessTexture") ==
            "diffuseRoughnessMap");
    // The parameter's own name is not a field of this extension.
    REQUIRE(GLTFLoader::diffuseRoughnessParamForField("diffuseRoughness").empty());
    REQUIRE(GLTFLoader::diffuseRoughnessParamForField("").empty());
}

TEST_CASE("GLTFLoader writes a diffuse roughness as zero when absent") {
    // A written value is taken, including zero.
    REQUIRE(GLTFLoader::diffuseRoughness(true, 0.25) == 0.25);
    REQUIRE(GLTFLoader::diffuseRoughness(true, 0.0) == 0.0);
    // An absent one is zero rather than the material's own value.
    REQUIRE(GLTFLoader::diffuseRoughness(false, 0.25) == 0.0);
}

TEST_CASE("GLTFLoader writes a dispersion as zero when absent") {
    // The field carries no suffix in the file, so the value passes straight through.
    REQUIRE(GLTFLoader::dispersion(true, 0.5) == 0.5);
    REQUIRE(GLTFLoader::dispersion(true, 0.0) == 0.0);
    // An absent one is zero.
    REQUIRE(GLTFLoader::dispersion(false, 0.5) == 0.0);
}

TEST_CASE("GLTFLoader defaults the two the same way") {
    // Both are always written, so both read as zero when the file says nothing; the two agree
    // even though one field is spelled with a suffix and the other without.
    REQUIRE(GLTFLoader::diffuseRoughness(false, 1.0) == GLTFLoader::dispersion(false, 1.0));
    REQUIRE(GLTFLoader::diffuseRoughness(false, 1.0) == 0.0);
}
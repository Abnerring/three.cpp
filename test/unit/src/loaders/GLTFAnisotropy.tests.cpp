#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader renames an anisotropy field") {
    // The strength drops a suffix that is not the word the other extensions use.
    REQUIRE(GLTFLoader::anisotropyParamForField("anisotropyStrength") == "anisotropy");
    // The rotation keeps its name exactly.
    REQUIRE(GLTFLoader::anisotropyParamForField("anisotropyRotation") == "anisotropyRotation");
    // The texture gains a map suffix.
    REQUIRE(GLTFLoader::anisotropyParamForField("anisotropyTexture") == "anisotropyMap");
}

TEST_CASE("GLTFLoader does not assume the factor suffix") {
    // A rule that dropped the word factor would map this, but the extension does not use it.
    REQUIRE(GLTFLoader::anisotropyParamForField("anisotropyFactor").empty());
    // The parameter's own name is not a field either.
    REQUIRE(GLTFLoader::anisotropyParamForField("anisotropy").empty());
    REQUIRE(GLTFLoader::anisotropyParamForField("").empty());
}

TEST_CASE("GLTFLoader sets an anisotropy number only when written") {
    // A written zero is a written value.
    const auto zero = GLTFLoader::anisotropyValue(true, 0.0);
    REQUIRE(zero.present);
    REQUIRE(zero.value == 0.0);
    // A written half is passed on.
    const auto half = GLTFLoader::anisotropyValue(true, 0.5);
    REQUIRE(half.present);
    REQUIRE(half.value == 0.5);
    // A missing one leaves the material alone.
    const auto absent = GLTFLoader::anisotropyValue(false, 0.5);
    REQUIRE_FALSE(absent.present);
}
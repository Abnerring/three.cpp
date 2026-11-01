#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader renames an iridescence field") {
    // A factor drops its suffix.
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceFactor") == "iridescence");
    // A texture gains a map suffix.
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceTexture") == "iridescenceMap");
    // The index of refraction changes case: a lower case o in the file, an upper case one on
    // the port's side.
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceIor") == "iridescenceIOR");
    // The three are different names.
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceIor") != "iridescenceIor");
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceTexture") !=
            GLTFLoader::iridescenceParamForField("iridescenceFactor"));
}

TEST_CASE("GLTFLoader names no parameter for an unknown iridescence field") {
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescence").empty());
    REQUIRE(GLTFLoader::iridescenceParamForField("iridescenceIOR").empty());
    REQUIRE(GLTFLoader::iridescenceParamForField("").empty());
}

TEST_CASE("GLTFLoader sets an iridescence number only when written") {
    // A written value, including zero, is passed on.
    const auto given = GLTFLoader::iridescenceValue(true, 0.0);
    REQUIRE(given.present);
    REQUIRE(given.value == 0.0);
    const auto half = GLTFLoader::iridescenceValue(true, 0.5);
    REQUIRE(half.present);
    REQUIRE(half.value == 0.5);
    // One that was not written leaves the material alone.
    const auto absent = GLTFLoader::iridescenceValue(false, 0.5);
    REQUIRE_FALSE(absent.present);
}
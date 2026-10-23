#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader keeps an attribute the geometry already has") {
    // A geometry that does not have it gets it.
    REQUIRE(GLTFLoader::shouldSetAttribute(false));
    // One that does keeps what it has, which is how a compressed primitive's attributes win.
    REQUIRE_FALSE(GLTFLoader::shouldSetAttribute(true));
}

TEST_CASE("GLTFLoader sets an index only when there is one to set") {
    // No index accessor in the file: nothing to set.
    REQUIRE_FALSE(GLTFLoader::shouldSetIndex(false, false));
    // An index accessor and an unindexed geometry: set it.
    REQUIRE(GLTFLoader::shouldSetIndex(true, false));
    // An already indexed geometry keeps its index.
    REQUIRE_FALSE(GLTFLoader::shouldSetIndex(true, true));
}

TEST_CASE("GLTFLoader warns about vertex colours in another colour space") {
    // The linear space needs no warning.
    REQUIRE(GLTFLoader::colourSpaceWarning("srgb-linear", true).empty());
    // Another space does, and the message names both.
    const auto warning = GLTFLoader::colourSpaceWarning("srgb", true);
    REQUIRE(warning ==
            "THREE.GLTFLoader: Converting vertex colors from \"srgb-linear\" to \"srgb\" not "
            "supported.");
    // Without a colour attribute there is nothing to convert and nothing to say.
    REQUIRE(GLTFLoader::colourSpaceWarning("srgb", false).empty());
    // The name of the working space is reported as it stands.
    const auto other = GLTFLoader::colourSpaceWarning("display-p3", true);
    REQUIRE(other.find("\"display-p3\"") != std::string::npos);
    REQUIRE(other.find("srgb-linear") != std::string::npos);
}
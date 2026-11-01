#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader fills in the sampler defaults") {
    // A sampler that names nothing gets the four defaults.
    const auto defaults = GLTFLoader::samplerSettings("", "", "", "");
    REQUIRE(defaults.magFilter == "LinearFilter");
    REQUIRE(defaults.minFilter == "LinearMipmapLinearFilter");
    REQUIRE(defaults.wrapS == "RepeatWrapping");
    REQUIRE(defaults.wrapT == "RepeatWrapping");
    // The two filters default differently, which is the point of the pair.
    REQUIRE(defaults.magFilter != defaults.minFilter);
}

TEST_CASE("GLTFLoader keeps the values a sampler names") {
    const auto named = GLTFLoader::samplerSettings("NearestFilter", "NearestMipmapNearestFilter",
                                                  "ClampToEdgeWrapping",
                                                  "MirroredRepeatWrapping");
    REQUIRE(named.magFilter == "NearestFilter");
    REQUIRE(named.minFilter == "NearestMipmapNearestFilter");
    REQUIRE(named.wrapS == "ClampToEdgeWrapping");
    REQUIRE(named.wrapT == "MirroredRepeatWrapping");
}

TEST_CASE("GLTFLoader fills in only what is missing") {
    // One filter given, the other defaulted.
    const auto half = GLTFLoader::samplerSettings("NearestFilter", "", "", "");
    REQUIRE(half.magFilter == "NearestFilter");
    REQUIRE(half.minFilter == "LinearMipmapLinearFilter");
    // One axis given, the other defaulted.
    const auto axis = GLTFLoader::samplerSettings("", "", "ClampToEdgeWrapping", "");
    REQUIRE(axis.wrapS == "ClampToEdgeWrapping");
    REQUIRE(axis.wrapT == "RepeatWrapping");
}

TEST_CASE("GLTFLoader decides whether mipmaps are needed") {
    // A filter that looks at more than one level needs them.
    REQUIRE(GLTFLoader::needsMipmaps(false, "LinearMipmapLinearFilter"));
    REQUIRE(GLTFLoader::needsMipmaps(false, "NearestMipmapNearestFilter"));
    // A single level filter does not.
    REQUIRE_FALSE(GLTFLoader::needsMipmaps(false, "LinearFilter"));
    REQUIRE_FALSE(GLTFLoader::needsMipmaps(false, "NearestFilter"));
}

TEST_CASE("GLTFLoader never asks a compressed texture for mipmaps") {
    // The levels are already in the file, whatever the filter says.
    REQUIRE_FALSE(GLTFLoader::needsMipmaps(true, "LinearMipmapLinearFilter"));
    REQUIRE_FALSE(GLTFLoader::needsMipmaps(true, "LinearFilter"));
}
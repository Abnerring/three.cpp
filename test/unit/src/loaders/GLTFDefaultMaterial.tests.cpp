#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFLoader starts a material fully metallic and rough") {
    const auto settings = GLTFLoader::defaultMaterialSettings();
    // Both are one, not a half.
    REQUIRE(settings.metalness == 1.0);
    REQUIRE(settings.roughness == 1.0);
    // White, with a black emissive.
    REQUIRE(settings.colour[0] == 1.0);
    REQUIRE(settings.colour[1] == 1.0);
    REQUIRE(settings.colour[2] == 1.0);
    REQUIRE(settings.emissive[0] == 0.0);
    REQUIRE(settings.emissive[2] == 0.0);
    // Opaque, depth tested and front facing.
    REQUIRE_FALSE(settings.transparent);
    REQUIRE(settings.depthTest);
    REQUIRE(settings.side == "FrontSide");
}

TEST_CASE("GLTFLoader caches the default material under one key") {
    REQUIRE(std::string(GLTFLoader::kDefaultMaterialKey) == "DefaultMaterial");
}

TEST_CASE("GLTFLoader keeps the unknown extensions") {
    // One known and one not: only the unknown one is kept.
    const auto unknown = GLTFLoader::unknownExtensions({"KHR_materials_unlit", "VENDOR_thing"},
                                                       {"KHR_materials_unlit"});
    REQUIRE(unknown.size() == 1);
    REQUIRE(unknown[0] == "VENDOR_thing");
    // Nothing unknown gives nothing.
    REQUIRE(GLTFLoader::unknownExtensions({"A", "B"}, {"A", "B"}).empty());
    // Nothing known keeps them all, in order.
    const auto all = GLTFLoader::unknownExtensions({"A", "B"}, {});
    REQUIRE(all.size() == 2);
    REQUIRE(all[0] == "A");
    REQUIRE(all[1] == "B");
}

TEST_CASE("GLTFLoader keeps an extension the loader has never heard of") {
    // The real list is built by the loader, so a name made up here is certainly unknown.
    const auto unknown =
        GLTFLoader::unknownExtensions({"VENDOR_made_up"}, GLTFLoader::knownExtensions());
    REQUIRE(unknown.size() == 1);
    // A name from the real list is known and therefore not kept.
    const auto known = GLTFLoader::unknownExtensions(
        {"KHR_draco_mesh_compression"}, GLTFLoader::knownExtensions());
    REQUIRE(known.empty());
}
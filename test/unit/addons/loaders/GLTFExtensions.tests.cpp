#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <set>
#include <string>

TEST_CASE("GLTFLoader lists twenty four extensions") {
    // The source lists twenty four names.
    REQUIRE(GLTFLoader::knownExtensionCount() == 24);
    REQUIRE(GLTFLoader::knownExtensions().size() == 24);
    // The first and the last are where the source puts them.
    REQUIRE(GLTFLoader::knownExtensions().front() == "KHR_binary_glTF");
    REQUIRE(GLTFLoader::knownExtensions().back() == "EXT_mesh_gpu_instancing");
}

TEST_CASE("GLTFLoader recognises an extension name") {
    // A name in the list counts.
    REQUIRE(GLTFLoader::isKnownExtension("KHR_draco_mesh_compression"));
    REQUIRE(GLTFLoader::isKnownExtension("KHR_materials_unlit"));
    REQUIRE(GLTFLoader::isKnownExtension("EXT_texture_webp"));
    // One that is not in the list does not.
    REQUIRE_FALSE(GLTFLoader::isKnownExtension("KHR_not_a_thing"));
    REQUIRE_FALSE(GLTFLoader::isKnownExtension(""));
    // The name is compared exactly, so the case matters.
    REQUIRE_FALSE(GLTFLoader::isKnownExtension("khr_draco_mesh_compression"));
}

TEST_CASE("GLTFLoader lists each extension once") {
    // A duplicate would make the count wrong and the lookup ambiguous.
    std::set<std::string> unique(GLTFLoader::knownExtensions().begin(),
                                 GLTFLoader::knownExtensions().end());
    REQUIRE(unique.size() == GLTFLoader::knownExtensionCount());
}

TEST_CASE("GLTFLoader names both compression extensions") {
    // Two spellings of the meshopt extension exist and both are listed.
    REQUIRE(GLTFLoader::isKnownExtension("EXT_meshopt_compression"));
    REQUIRE(GLTFLoader::isKnownExtension("KHR_meshopt_compression"));
    // They are different names and both count towards the total.
    REQUIRE(std::string("EXT_meshopt_compression") != "KHR_meshopt_compression");
}
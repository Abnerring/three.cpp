#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFLoader finds a material extension") {
    const std::vector<std::string> names{"KHR_materials_unlit", "KHR_materials_volume"};
    // A name in the list counts.
    REQUIRE(GLTFLoader::hasMaterialExtension(names, "KHR_materials_unlit"));
    REQUIRE(GLTFLoader::hasMaterialExtension(names, "KHR_materials_volume"));
    // One that is not does not.
    REQUIRE_FALSE(GLTFLoader::hasMaterialExtension(names, "KHR_materials_sheen"));
    // A material with no extensions at all names none.
    REQUIRE_FALSE(GLTFLoader::hasMaterialExtension({}, "KHR_materials_unlit"));
    // An empty name is not found either.
    REQUIRE_FALSE(GLTFLoader::hasMaterialExtension(names, ""));
}

TEST_CASE("GLTFLoader picks the material type") {
    // Only the unlit extension changes it, and then to a material that ignores lighting.
    REQUIRE(GLTFLoader::materialTypeName(true) == "MeshBasicMaterial");
    REQUIRE(GLTFLoader::materialTypeName(false) == "MeshStandardMaterial");
    REQUIRE(GLTFLoader::materialTypeName(true) != GLTFLoader::materialTypeName(false));
}

TEST_CASE("GLTFLoader copies an emissive strength") {
    // A strength that was written is passed on.
    const auto given = GLTFLoader::emissiveIntensity(true, 2.5);
    REQUIRE(given.present);
    REQUIRE(given.value == 2.5);
    // One that was not leaves the material's own intensity alone, which the flag records.
    const auto absent = GLTFLoader::emissiveIntensity(false, 2.5);
    REQUIRE_FALSE(absent.present);
    // A strength of zero is still a strength, so it is applied rather than skipped.
    const auto zero = GLTFLoader::emissiveIntensity(true, 0.0);
    REQUIRE(zero.present);
    REQUIRE(zero.value == 0.0);
}
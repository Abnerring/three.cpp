#include "loaders/GLTFGaussianSplatLoaderExtension.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFGaussianSplatLoaderExtension maps a semantic to an attribute name") {
    // Position is the only explicit entry.
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("POSITION") == "position");
    // Everything else is its own name lower cased.
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("SCALE") == "scale");
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("ROTATION") == "rotation");
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("OPACITY") == "opacity");
    // An underscored extension semantic keeps its underscore.
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("_SH0") == "_sh0");
    // Something already lower case is unchanged.
    REQUIRE(GLTFGaussianSplatLoaderExtension::attributeName("color") == "color");
}

TEST_CASE("GLTFGaussianSplatLoaderExtension detects its extension by name") {
    REQUIRE(GLTFGaussianSplatLoaderExtension::isGaussianSplatPrimitive(
        {"KHR_gaussian_splatting"}));
    REQUIRE(GLTFGaussianSplatLoaderExtension::isGaussianSplatPrimitive(
        {"KHR_materials_unlit", "KHR_gaussian_splatting"}));
    // Other extensions do not count.
    REQUIRE_FALSE(GLTFGaussianSplatLoaderExtension::isGaussianSplatPrimitive(
        {"KHR_materials_unlit"}));
    REQUIRE_FALSE(GLTFGaussianSplatLoaderExtension::isGaussianSplatPrimitive({}));
}

TEST_CASE("GLTFGaussianSplatLoaderExtension names both failures") {
    // A semantic the primitive does not declare.
    REQUIRE(GLTFGaussianSplatLoaderExtension::requiredMessage("SCALE") ==
            "THREE.GLTFGaussianSplatLoaderExtension: KHR_gaussian_splatting requires SCALE.");
    // One it declares but whose data never arrived.
    REQUIRE(GLTFGaussianSplatLoaderExtension::missingMessage("ROTATION") ==
            "THREE.GLTFGaussianSplatLoaderExtension: KHR_gaussian_splatting attribute ROTATION "
            "was not loaded.");
}

#include "nodes/core/SceneProperties.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SceneProperties holds the defaults of its three values") {
    // A scene with nothing set reads nought blurriness and unit intensity.
    REQUIRE(SceneProperties::kDefaultBlurriness == 0.0);
    REQUIRE(SceneProperties::kDefaultIntensity == 1.0);
    // The two are not the same number, which is why both are written out.
    REQUIRE(SceneProperties::kDefaultBlurriness != SceneProperties::kDefaultIntensity);
    REQUIRE(std::string(SceneProperties::kUvMappingName) == "UVMapping");
}

TEST_CASE("SceneProperties applies the rotation to a texture under another mapping") {
    // No background at all gives no rotation.
    REQUIRE_FALSE(SceneProperties::rotationApplies(false, false, false, false, false));
    // A texture under a mapping other than plain takes it, which is the cube case.
    REQUIRE(SceneProperties::rotationApplies(true, true, false, false, false));
    // The same texture under the plain mapping does not, so it keeps an identity matrix even
    // though the scene names a rotation.
    REQUIRE_FALSE(SceneProperties::rotationApplies(true, true, true, false, false));
    // Those two differ only in the mapping, which is the property worth checking.
    REQUIRE(SceneProperties::rotationApplies(true, true, false, false, false) !=
            SceneProperties::rotationApplies(true, true, true, false, false));
}

TEST_CASE("SceneProperties applies the rotation to a background node") {
    // A background node takes it whatever the texture cases said.
    REQUIRE(SceneProperties::rotationApplies(false, false, false, true, true));
    // A background that is present but not a node does not.
    REQUIRE_FALSE(SceneProperties::rotationApplies(false, false, false, true, false));
    // A node and a plain texture together still apply, since either case suffices.
    REQUIRE(SceneProperties::rotationApplies(true, true, true, true, true));
}

TEST_CASE("SceneProperties ignores a background that is not a texture") {
    // A present background that is not a texture takes no rotation, since the first condition
    // asks for a texture as well as a mapping.
    REQUIRE_FALSE(SceneProperties::rotationApplies(true, false, false, false, false));
    // And a texture that is absent cannot be one, however the two flags are given.
    REQUIRE_FALSE(SceneProperties::rotationApplies(false, true, false, false, false));
}
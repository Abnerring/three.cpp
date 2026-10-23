#include "nodes/core/MaterialProperties.h"

#include "catch.hpp"

using Presence = MaterialProperties::Presence;

TEST_CASE("MaterialProperties picks the intensity with the loose test") {
    // A map that is true in the language's sense picks the material's own intensity.
    REQUIRE(MaterialProperties::intensitySource(Presence::Present) ==
            MaterialProperties::IntensitySource::Material);
    // Null and unset both fall back to the scene, which is what the loose test gives.
    REQUIRE(MaterialProperties::intensitySource(Presence::Null) ==
            MaterialProperties::IntensitySource::Scene);
    REQUIRE(MaterialProperties::intensitySource(Presence::Undefined) ==
            MaterialProperties::IntensitySource::Scene);
    // The two absences agree here, unlike in the rotation below.
    REQUIRE(MaterialProperties::intensitySource(Presence::Null) ==
            MaterialProperties::intensitySource(Presence::Undefined));
}

TEST_CASE("MaterialProperties counts an unset scene environment as present") {
    // The scene's own value is tested against null only, and an unset value is not null, so a
    // scene that never set one still counts as having it.
    REQUIRE(MaterialProperties::sceneHasEnvironment(Presence::Present, false, false));
    REQUIRE_FALSE(MaterialProperties::sceneHasEnvironment(Presence::Null, false, false));
    REQUIRE(MaterialProperties::sceneHasEnvironment(Presence::Undefined, false, false));
    // That last one is the point: unset and present agree, and only null does not.
    REQUIRE(MaterialProperties::sceneHasEnvironment(Presence::Undefined, false, false) ==
            MaterialProperties::sceneHasEnvironment(Presence::Present, false, false));
}

TEST_CASE("MaterialProperties lets a node stand in for the scene environment") {
    // A scene with no value at all but with a node still has an environment.
    REQUIRE(MaterialProperties::sceneHasEnvironment(Presence::Null, true, true));
    // A node that is not a node does not.
    REQUIRE_FALSE(MaterialProperties::sceneHasEnvironment(Presence::Null, true, false));
    REQUIRE_FALSE(MaterialProperties::sceneHasEnvironment(Presence::Null, false, true));
}

TEST_CASE("MaterialProperties' rotation test disagrees with its intensity test") {
    // The rotation uses the scene's only when the material's map is exactly null. So a material
    // whose map was never set keeps its own rotation, while the same unset map falls back to the
    // scene for the intensity. This is where the two tests part company.
    REQUIRE(MaterialProperties::rotationSource(Presence::Null, true) ==
            MaterialProperties::RotationSource::Scene);
    REQUIRE(MaterialProperties::rotationSource(Presence::Undefined, true) ==
            MaterialProperties::RotationSource::Material);
    // And the intensity took the scene for both, so the two differ on the unset case.
    REQUIRE(MaterialProperties::intensitySource(Presence::Null) ==
            MaterialProperties::intensitySource(Presence::Undefined));
    REQUIRE(MaterialProperties::rotationSource(Presence::Null, true) !=
            MaterialProperties::rotationSource(Presence::Undefined, true));
    // With no scene environment the material's own applies whatever its map says.
    REQUIRE(MaterialProperties::rotationSource(Presence::Null, false) ==
            MaterialProperties::RotationSource::Material);
    REQUIRE(MaterialProperties::rotationSource(Presence::Present, true) ==
            MaterialProperties::RotationSource::Material);
}
#include "nodes/core/MaterialProperties.h"

MaterialProperties::IntensitySource MaterialProperties::intensitySource(Presence envMap) {
    // The loose test: only a value that is true in the language's sense picks the material's own
    // intensity, so null and unset both fall back to the scene.
    return envMap == Presence::Present ? IntensitySource::Material : IntensitySource::Scene;
}

bool MaterialProperties::sceneHasEnvironment(Presence environment, bool hasEnvironmentNode,
                                             bool nodeIsNode) {
    // Not null, which an unset value is not, so an unset scene value still counts.
    const bool byValue = environment != Presence::Null;
    const bool byNode = hasEnvironmentNode && nodeIsNode;
    return byValue || byNode;
}

MaterialProperties::RotationSource MaterialProperties::rotationSource(Presence envMap,
                                                                     bool sceneHasEnvironment) {
    // The strict test: the scene's rotation is used only when the material's map is exactly
    // null, so a material whose map was never set keeps its own rotation.
    if (sceneHasEnvironment && envMap == Presence::Null) return RotationSource::Scene;
    // Otherwise the material's own, which is absent only when the material never set one.
    if (envMap == Presence::Present || envMap == Presence::Undefined) {
        return RotationSource::Material;
    }
    return RotationSource::Material;
}

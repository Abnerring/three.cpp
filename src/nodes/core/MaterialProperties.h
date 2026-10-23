#pragma once
#include <string>

// Port of the conditions in src/nodes/accessors/MaterialProperties.js at the pinned revision.
// The module refreshes three uniforms from the material in use, and what is portable is how it
// decides which of two sources each value comes from. The three decisions test for absence in
// three different ways, which is the thing worth writing down.
class MaterialProperties {
  public:
    // What an absent value is taken to be. The three tests below treat it differently, so this
    // enumerates the states rather than collapsing them into one.
    enum class Presence {
        // The value is there and is true in the language's sense.
        Present,
        // The value is null.
        Null,
        // The value was never set at all.
        Undefined
    };

    // Which intensity the environment uses. The material's own is chosen when its environment
    // map is true in the language's sense; a map that is null or absent is not, so both fall
    // back to the scene's. This is the loose of the three tests.
    enum class IntensitySource { Material, Scene };

    static IntensitySource intensitySource(Presence envMap);

    // Whether the scene counts as having an environment. The test asks whether the scene's
    // value is not null, and an unset value is not null, so a scene that never set one still
    // counts as having it. This is the third of the three tests and the one easiest to mistake
    // for its opposite.
    static bool sceneHasEnvironment(Presence environment, bool hasEnvironmentNode, bool nodeIsNode);

    // Which rotation applies. The scene's is used when the scene counts as having an environment
    // and the material's map is exactly null, so a material whose map was never set takes its own
    // rotation rather than the scene's. This is the strict test, and it disagrees with the first
    // one for exactly that case.
    enum class RotationSource { Scene, Material, Identity };

    static RotationSource rotationSource(Presence envMap, bool sceneHasEnvironment);
};

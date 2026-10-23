#pragma once
#include <string>

// Port of the parts of src/nodes/accessors/SceneProperties.js at the pinned revision that decide
// what the scene's own values are and when its rotation applies. The three values are uniforms
// refreshed every render; what matters here is which of them is conditional.
class SceneProperties {
  public:
    // The defaults the three uniforms start at. Blurriness begins at nought and intensity at
    // one, which are the values a scene with nothing set on it should read.
    static constexpr double kDefaultBlurriness = 0.0;
    static constexpr double kDefaultIntensity = 1.0;

    // Whether the scene's rotation should be applied to its background. It applies only when
    // the background is a texture under some mapping other than the plain one, or when a
    // background node is present; a plain uv mapped texture therefore ignores the rotation
    // entirely and keeps an identity matrix.
    static bool rotationApplies(bool hasBackground, bool backgroundIsTexture, bool mappingIsUv,
                                bool hasBackgroundNode, bool backgroundNodeIsNode);

    // The name the plain uv mapping goes by, so that the test above can be read against it.
    static constexpr const char* kUvMappingName = "UVMapping";
};

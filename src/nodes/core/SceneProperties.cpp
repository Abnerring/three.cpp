#include "nodes/core/SceneProperties.h"

bool SceneProperties::rotationApplies(bool hasBackground, bool backgroundIsTexture,
                                      bool mappingIsUv, bool hasBackgroundNode,
                                      bool backgroundNodeIsNode) {
    // A background that is a texture under a mapping other than the plain one takes the
    // rotation; a plain uv mapped one does not, and keeps an identity matrix.
    const bool textureCase = hasBackground && backgroundIsTexture && !mappingIsUv;
    // A background node takes it too, provided that node really is a node.
    const bool nodeCase = hasBackgroundNode && backgroundNodeIsNode;
    return textureCase || nodeCase;
}

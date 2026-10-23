#include "nodes/core/ViewportDepthTextureNode.h"

ViewportDepthTextureNode::Source ViewportDepthTextureNode::sourceFor(bool textureWasGiven) {
    // A given texture is used as it stands; only an absent one falls back to the shared one.
    return textureWasGiven ? Source::Given : Source::Shared;
}

bool ViewportDepthTextureNode::sharesOneDepthTexture() {
    return true;
}

bool ViewportDepthTextureNode::overridesReferenceUpdate() {
    // The ordinary behaviour is kept, unlike the shared sibling which overrides it.
    return false;
}

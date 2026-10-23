#include "nodes/core/ViewportDepthTexture.h"

namespace {
// The one shared texture, made the first time a node asks for one.
std::uint64_t gSharedTextureId = 0;
std::uint32_t gNextId = 1;
std::uint32_t gDefaultTexturesMade = 0;
std::uint32_t gNodesServed = 0;
}

std::uint32_t ViewportDepthTexture::defaultTextureCount() {
    return gDefaultTexturesMade;
}

std::uint32_t ViewportDepthTexture::nodesServed() {
    return gNodesServed;
}

std::uint64_t ViewportDepthTexture::textureId(bool hasOwnTexture, std::uint64_t ownTextureId) {
    // A texture that was given is used as it stands.
    if (hasOwnTexture) {
        gNodesServed += 1;
        return ownTextureId;
    }
    // Otherwise the shared one, made if this is the first ask.
    gNodesServed += 1;
    if (gSharedTextureId == 0) {
        gSharedTextureId = gNextId;
        gNextId += 1;
        gDefaultTexturesMade += 1;
    }
    return gSharedTextureId;
}

void ViewportDepthTexture::reset() {
    gSharedTextureId = 0;
    gNextId = 1;
    gDefaultTexturesMade = 0;
    gNodesServed = 0;
}

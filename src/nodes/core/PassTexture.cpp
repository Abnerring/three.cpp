#include "nodes/core/PassTexture.h"

bool PassTexture::isCacheable() {
    // Yes, which is the opposite of the other nodes in this layer.
    return true;
}

int PassTexture::textureCount(bool isMultiple) {
    return isMultiple ? kMultipleTextureCount : 1;
}

bool PassTexture::cloneSharesTexture() {
    // The clone is built from the same pass and the same value, so it is not a copy.
    return true;
}

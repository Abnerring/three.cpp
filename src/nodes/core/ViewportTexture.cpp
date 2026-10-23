#include "nodes/core/ViewportTexture.h"

#include <string>

bool ViewportTexture::filterAndMipmapsAgree() {
    // The filter reads mipmaps and the setting forbids making them, so they do not agree.
    const bool filterReadsMipmaps = std::string(kMinFilter) == "LinearMipmapLinearFilter";
    return !(filterReadsMipmaps && !generatesMipmaps);
}

bool ViewportTexture::makesDefaultTexture(bool hasGivenTexture) {
    // A texture is made only when none was given.
    return !hasGivenTexture;
}

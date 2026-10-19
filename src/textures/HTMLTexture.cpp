#include "textures/HTMLTexture.h"

HTMLTexture::HTMLTexture(Image element, double mapping, double wrapS, double wrapT, double magFilter,
                         double minFilter, double format, double type, double anisotropy, std::string colorSpace)
    : Texture(std::move(element), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy,
              std::move(colorSpace)) {
    // Upstream turns mipmaps off for an element backed texture and raises the update flag.
    generateMipmaps = false;
    needsUpdate = true;
}
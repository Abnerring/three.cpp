#include "textures/CanvasTexture.h"

CanvasTexture::CanvasTexture(Image canvas, double mapping, double wrapS, double wrapT, double magFilter,
                             double minFilter, double format, double type, double anisotropy,
                             std::string colorSpace)
    : Texture(std::move(canvas), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy,
              std::move(colorSpace)) {
    // Upstream raises this in the constructor rather than leaving it to the caller.
    needsUpdate = true;
}
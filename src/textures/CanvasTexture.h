#pragma once
#include "textures/Texture.h"

// Port of src/textures/CanvasTexture.js at the pinned revision.
//
// Upstream takes an HTMLCanvasElement, and there is no DOM here, so the host supplies a texture image instead:
// the same {width, height, data} descriptor the image loader decoder produces. Everything else is the upstream
// class exactly - it is the base texture with the canvas flag set and needsUpdate raised in the constructor,
// because a canvas is ready to sample the moment it exists.
class CanvasTexture : public Texture {
  public:
    const bool isCanvasTexture = true;
    explicit CanvasTexture(Image canvas = DEFAULT_IMAGE, double mapping = DEFAULT_MAPPING,
                           double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                           double magFilter = LinearFilter, double minFilter = LinearMipmapLinearFilter,
                           double format = RGBAFormat, double type = UnsignedByteType,
                           double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
};
#pragma once
#include "textures/Texture.h"

// Port of src/textures/HTMLTexture.js at the pinned revision.
//
// Upstream wraps an HTML element and wires the parent element paint hooks - requestPaint plus an onpaint
// handler that raises needsUpdate. Those are host concerns with no counterpart here, and the class documents
// that rather than pretending: the host supplies a texture image and is responsible for raising needsUpdate
// again when the pixels change. Everything else matches: the flag, mipmaps off, and the update raised on
// construction.
class HTMLTexture : public Texture {
  public:
    const bool isHTMLTexture = true;
    explicit HTMLTexture(Image element = DEFAULT_IMAGE, double mapping = DEFAULT_MAPPING,
                         double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                         double magFilter = LinearFilter, double minFilter = LinearMipmapLinearFilter,
                         double format = RGBAFormat, double type = UnsignedByteType,
                         double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
};
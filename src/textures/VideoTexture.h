#pragma once
#include "textures/Texture.h"

// Port of src/textures/VideoTexture.js at the pinned revision.
//
// The video is a host object, so it is carried in the texture image data slot and kept here as a handle: the
// texture needs it in update and dispose, which upstream reaches through image. Both filters default to linear
// rather than the base mipmapped pair, mipmaps are off, and the frame callback the host may provide is
// registered so the texture raises its update flag when a new frame arrives. A host video without that callback
// is polled through readyState in update instead, which is the upstream fallback.
class VideoTexture : public Texture {
  public:
    const bool isVideoTexture = true;
    explicit VideoTexture(JSValue video = {}, double mapping = DEFAULT_MAPPING,
                          double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                          double magFilter = LinearFilter, double minFilter = LinearFilter,
                          double format = RGBAFormat, double type = UnsignedByteType,
                          double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
    VideoTexture& update();
    JSValue video;
    double requestVideoFrameCallbackId = 0;
};
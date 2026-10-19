#pragma once
#include "textures/VideoTexture.h"

// Port of src/textures/VideoFrameTexture.js at the pinned revision.
//
// A video texture that is fed one frame at a time rather than a playing element: setFrame stores the frame and
// raises the update flag, update does nothing because there is no element to poll, and clone restores the base
// texture behaviour. The frame itself is a host object and is kept in the image slot like the video is.
class VideoFrameTexture : public VideoTexture {
  public:
    const bool isVideoFrameTexture = true;
    explicit VideoFrameTexture(double mapping = DEFAULT_MAPPING, double wrapS = ClampToEdgeWrapping,
                               double wrapT = ClampToEdgeWrapping, double magFilter = LinearFilter,
                               double minFilter = LinearFilter, double format = RGBAFormat,
                               double type = UnsignedByteType, double anisotropy = DEFAULT_ANISOTROPY,
                               std::string colorSpace = NoColorSpace);
    VideoFrameTexture& setFrame(JSValue frame = {});
    VideoFrameTexture& update();
    std::shared_ptr<Texture> clone() const override;
};
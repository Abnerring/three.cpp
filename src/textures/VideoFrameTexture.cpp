#include "textures/VideoFrameTexture.h"

VideoFrameTexture::VideoFrameTexture(double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                                     double format, double type, double anisotropy, std::string colorSpace)
    : VideoTexture(JSValue::object(), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy,
                   std::move(colorSpace)) {
}

VideoFrameTexture& VideoFrameTexture::setFrame(JSValue frame) {
    video = frame;
    // The frame goes into the image slot and the update flag is raised, which is all upstream does.
    auto image = std::make_shared<TextureImage>(TextureImage::Data{frame}, 0.0, 0.0);
    this->image = image;
    needsUpdate = true;
    return *this;
}

VideoFrameTexture& VideoFrameTexture::update() { return *this; }

std::shared_ptr<Texture> VideoFrameTexture::clone() const {
    auto copy = std::make_shared<VideoFrameTexture>();
    copy->copy(*this);
    return copy;
}
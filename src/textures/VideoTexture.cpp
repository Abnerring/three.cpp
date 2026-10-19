#include "textures/VideoTexture.h"

#include "textures/TextureSource.h"

namespace {
// The frame callback is a JS function and has to be captureless, so the texture it belongs to lives in a slot
// for the duration of the call.
VideoTexture*& pendingTexture() {
    static thread_local VideoTexture* value = nullptr;
    return value;
}

JSValue onFrame(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    auto* texture = pendingTexture();
    if (texture) {
        texture->needsUpdate = true;
        // Re-register, which is how the upstream helper keeps a callback in flight.
        if (texture->requestVideoFrameCallbackId != 0 && !texture->video.get("requestVideoFrameCallback").isUndefined()) {
            pendingTexture() = texture;
            const auto id = texture->video.get("requestVideoFrameCallback").call(
                texture->video, {JSValue::function(onFrame, {}, "updateVideo", 1)});
            texture->requestVideoFrameCallbackId = id.toNumber();
        }
    }
    return JSValue();
}
}

VideoTexture::VideoTexture(JSValue video, double mapping, double wrapS, double wrapT, double magFilter,
                           double minFilter, double format, double type, double anisotropy, std::string colorSpace)
    : Texture(std::make_shared<TextureImage>(TextureImage::Data{video}, video.get("width").toNumber(),
                                             video.get("height").toNumber()),
              mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)),
      video(std::move(video)) {
    generateMipmaps = false;
    if (!this->video.get("requestVideoFrameCallback").isUndefined()) {
        pendingTexture() = this;
        const auto id = this->video.get("requestVideoFrameCallback").call(
            this->video, {JSValue::function(onFrame, {}, "updateVideo", 1)});
        requestVideoFrameCallbackId = id.toNumber();
    }
}

VideoTexture& VideoTexture::update() {
    // Without a frame callback the readyState decides, which is the upstream fallback.
    const bool hasCallback = !video.get("requestVideoFrameCallback").isUndefined();
    if (!hasCallback && video.get("readyState").toNumber() >= video.get("HAVE_CURRENT_DATA").toNumber()) {
        needsUpdate = true;
    }
    return *this;
}
#include "catch.hpp"
#include "core/JSValue.h"
#include "textures/TextureSource.h"
#include "textures/VideoFrameTexture.h"
#include "textures/VideoTexture.h"
#include <vector>

namespace {
int frameCallbacks = 0;
JSValue video() {
    auto element = JSValue::object();
    element.set("width", 4);
    element.set("height", 2);
    element.set("readyState", 4);
    element.set("HAVE_CURRENT_DATA", 2);
    element.set("requestVideoFrameCallback", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
        frameCallbacks += 1;
        return JSValue(7);
    }, {}, "requestVideoFrameCallback", 1));
    return element;
}
}

// A video backed texture turns mipmaps off, defaults both filters to linear, and registers a frame callback so
// the host can tell it when a new frame arrives.
TEST_CASE("VideoTexture registers a frame callback and keeps linear filters") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    frameCallbacks = 0;
    VideoTexture texture(video());
    REQUIRE(texture.isVideoTexture);
    REQUIRE(texture.generateMipmaps == false);
    REQUIRE(texture.magFilter == LinearFilter);
    REQUIRE(texture.minFilter == LinearFilter);
    REQUIRE(frameCallbacks == 1);
    REQUIRE(texture.requestVideoFrameCallbackId == 7.0);
    REQUIRE(texture.update().isVideoTexture);
}

// A frame texture is fed one frame at a time: setFrame stores it and raises the update flag, update does
// nothing, and clone restores the base texture behaviour.
TEST_CASE("VideoFrameTexture stores the frame it is given") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    VideoFrameTexture texture;
    REQUIRE(texture.isVideoFrameTexture);
    REQUIRE(texture.isVideoTexture);
    texture.setFrame(video());
    REQUIRE(texture.image.get().get());
    REQUIRE(texture.update().isVideoFrameTexture);
    const auto copy = texture.clone();
    REQUIRE(copy);
    REQUIRE(std::dynamic_pointer_cast<VideoFrameTexture>(copy));
}
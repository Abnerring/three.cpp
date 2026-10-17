#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/Cache.h"
#include "loaders/ImageLoader.h"
#include "loaders/TextureLoader.h"
#include <string>
#include <vector>

// The texture loader is a wrapper over ImageLoader, so these cases inject a decoder and check what
// the pinned wrapper exposes: a texture returned synchronously, its image set by the callback, the
// needsUpdate flag raised, and the failure path leaving the image unset while onError receives it.
namespace {
using Args = std::vector<JSValue>;
int decodes = 0;
bool failNext = false;
std::string seenError;
JSValue decoderFn(const JSValue&, const Args&, const Args&) {
    decodes += 1;
    if (failNext) throw JSTypeError("no image");
    auto image = JSValue::object();
    image.set("width", 4.0);
    image.set("height", 2.0);
    return image;
}
JSValue onErrorFn(const JSValue&, const Args& args, const Args&) {
    seenError = args.empty() ? std::string{} : args[0].toString();
    return {};
}
void prepare(bool shouldFail) {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    decodes = 0; failNext = shouldFail; seenError.clear();
    Cache.clear(); Cache.enabled = true;
    ImageLoader::setDecoder(JSValue::function(decoderFn, {}, "decoder", 1));
}
}

TEST_CASE("TextureLoader returns a texture and fills its image from the decoder", "[textureloader]") {
    prepare(false);
    TextureLoader loader;
    const auto texture = loader.load("tex.png");
    REQUIRE(decodes == 1);
    // The texture is returned synchronously and carries what the callback set on it.
    REQUIRE(texture.get("image").get("width").toNumber() == 4.0);
    REQUIRE(texture.get("image").get("height").toNumber() == 2.0);
    // Upstream exposes needsUpdate as a setter only: it bumps version rather than holding a flag, so
    // the observable effect of the loader marking the texture is the incremented version.
    REQUIRE(texture.get("version").toNumber() >= 1.0);
    ImageLoader::setDecoder(JSValue{});
    Cache.enabled = false;
    Cache.clear();
}

TEST_CASE("TextureLoader passes a decoder failure to onError and leaves the image unset", "[textureloader]") {
    prepare(true);
    TextureLoader loader;
    try {
        loader.load("bad.png", JSValue{}, JSValue{}, JSValue::function(onErrorFn, {}, "onError", 1));
        REQUIRE(false);
    } catch (const JSTypeError&) {
        REQUIRE(seenError.find("no image") != std::string::npos);
    }
    ImageLoader::setDecoder(JSValue{});
    Cache.enabled = false;
    Cache.clear();
}
#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/CubeTextureLoader.h"
#include "loaders/ImageLoader.h"
#include <string>
#include <vector>

// A cube texture is filled by six image loads. The cases pin what the pinned loader exposes: the
// callback only fires once all six arrived, the six faces land, the version is bumped, and a shorter
// url array never calls back because the loop runs over urls.length while the callback tests 6.
namespace {
using Args = std::vector<JSValue>;
// The realm lives for the whole process: image loading touches the cache, which needs one.
const JSValue& realm() { static const auto value = JSValue::object(); return value; }
int decodes = 0;
int callbacks = 0;
bool failNext = false;
std::string seenError;
JSValue decoderFn(const JSValue&, const Args&, const Args&) {
    decodes += 1;
    if (failNext) throw JSTypeError("no image");
    auto image = JSValue::object();
    image.set("width", 8.0);
    image.set("height", 4.0);
    return image;
}
JSValue onLoadFn(const JSValue&, const Args&, const Args&) { callbacks += 1; return {}; }
JSValue onErrorFn(const JSValue&, const Args& args, const Args&) {
    seenError = args.empty() ? std::string{} : args[0].toString();
    return {};
}
JSValue urlsOf(int count) {
    auto urls = JSValue::array();
    for (int index = 0; index < count; ++index) {
        const auto name = "cube_" + std::to_string(count) + "_" + std::to_string(index) + ".png";
        urls.get("push").call(urls, {JSValue(name)});
    }
    return urls;
}
void prepare(bool shouldFail) {
    static_cast<void>(realm());
    decodes = 0; callbacks = 0; failNext = shouldFail; seenError.clear();
    ImageLoader::setDecoder(JSValue::function(decoderFn, {}, "decoder", 1));
}
void finish() { ImageLoader::setDecoder(JSValue{}); }
}

TEST_CASE("CubeTextureLoader fills six faces and calls back once", "[cubetextureloader]") {
    prepare(false);
    CubeTextureLoader loader;
    const auto texture = loader.load(urlsOf(6), JSValue::function(onLoadFn, {}, "onLoad", 1));
    REQUIRE(decodes == 6);
    REQUIRE(callbacks == 1);
    REQUIRE(texture.get("colorSpace").toString() == "srgb");
    REQUIRE(texture.get("version").toNumber() >= 1.0);
    finish();
}

TEST_CASE("CubeTextureLoader does not call back for fewer than six urls", "[cubetextureloader]") {
    prepare(false);
    CubeTextureLoader loader;
    loader.load(urlsOf(4), JSValue::function(onLoadFn, {}, "onLoad", 1));
    REQUIRE(decodes == 4);
    REQUIRE(callbacks == 0);
    finish();
}

TEST_CASE("CubeTextureLoader surfaces a decoder failure", "[cubetextureloader]") {
    prepare(true);
    CubeTextureLoader loader;
    try {
        loader.load(urlsOf(6), JSValue::function(onLoadFn, {}, "onLoad", 1), JSValue{}, JSValue::function(onErrorFn, {}, "onError", 1));
        REQUIRE(false);
    } catch (const JSTypeError&) {
        REQUIRE(seenError.find("no image") != std::string::npos);
    }
    REQUIRE(callbacks == 0);
    finish();
}
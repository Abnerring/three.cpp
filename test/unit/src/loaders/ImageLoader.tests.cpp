#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/Cache.h"
#include "loaders/ImageLoader.h"
#include <string>
#include <vector>

// The decoder stands in for the DOM Image. Each case checks what the pinned loader would expose: the
// record handed to the callback, the cache hit that skips decoding, the failure path that reports to
// onError and leaves the cache empty, and the loud failure when no decoder exists.
namespace {
using Args = std::vector<JSValue>;
int decodes = 0;
double lastWidth = 0;
bool failNext = false;
std::string seenError;
JSValue decoderFn(const JSValue&, const Args& args, const Args&) {
    decodes += 1;
    if (failNext) throw JSTypeError("decoder says no");
    const auto request = args.empty() ? JSValue{} : args[0];
    auto image = JSValue::object();
    image.set("width", 2.0);
    image.set("height", 1.0);
    image.set("url", request.get("url"));
    lastWidth = image.get("width").toNumber();
    return image;
}
JSValue onLoadFn(const JSValue&, const Args& args, const Args&) {
    if (!args.empty()) lastWidth = args[0].get("width").toNumber();
    return {};
}
JSValue onErrorFn(const JSValue&, const Args& args, const Args&) {
    seenError = args.empty() ? std::string{} : args[0].toString();
    return {};
}
void reset() {
    decodes = 0; lastWidth = 0; failNext = false; seenError.clear();
    ImageLoader::setDecoder(JSValue::function(decoderFn, {}, "decoder", 1));
}
}

TEST_CASE("ImageLoader decodes through the injected decoder", "[imageloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    reset();
    // Cache is disabled by default, so a second load would decode again unless it is switched on.
    Cache.clear();
    Cache.enabled = true;
    ImageLoader loader;
    const auto image = loader.load("tex.png", JSValue::function(onLoadFn, {}, "onLoad", 1));
    REQUIRE(decodes == 1);
    REQUIRE(image.get("width").toNumber() == 2.0);
    REQUIRE(image.get("complete").toBoolean());
    REQUIRE(lastWidth == 2.0);
    ImageLoader::setDecoder(JSValue{});
}

TEST_CASE("ImageLoader answers a second load from the cache", "[imageloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    reset();
    // Cache is disabled by default, so a second load would decode again unless it is switched on.
    Cache.clear();
    Cache.enabled = true;
    ImageLoader loader;
    loader.load("same.png", JSValue::function(onLoadFn, {}, "onLoad", 1));
    REQUIRE(decodes == 1);
    loader.load("same.png", JSValue::function(onLoadFn, {}, "onLoad", 1));
    REQUIRE(decodes == 1);
    ImageLoader::setDecoder(JSValue{});
}

TEST_CASE("ImageLoader reports a decoder failure and leaves the cache empty", "[imageloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    reset();
    failNext = true;
    // Cache is disabled by default, so a second load would decode again unless it is switched on.
    Cache.clear();
    Cache.enabled = true;
    ImageLoader loader;
    REQUIRE_THROWS_AS(loader.load("bad.png", JSValue{}, JSValue{}, JSValue::function(onErrorFn, {}, "onError", 1)), JSTypeError);
    REQUIRE(seenError.find("decoder says no") != std::string::npos);
    ImageLoader::setDecoder(JSValue{});
}

TEST_CASE("ImageLoader fails loudly without a decoder", "[imageloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    ImageLoader::setDecoder(JSValue{});
    // Cache is disabled by default, so a second load would decode again unless it is switched on.
    Cache.clear();
    Cache.enabled = true;
    ImageLoader loader;
    REQUIRE_THROWS_AS(loader.load("nope.png"), JSTypeError);
}
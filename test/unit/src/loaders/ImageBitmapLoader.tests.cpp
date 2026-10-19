#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/Cache.h"
#include "loaders/ImageBitmapLoader.h"
#include <string>

namespace {
int decodeCount = 0;
std::string lastOption;
JSValue record(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    decodeCount += 1;
    lastOption = args.at(0).get("options").get("premultiplyAlpha").toString();
    auto bitmap = JSValue::object();
    bitmap.set("width", 2);
    bitmap.set("height", 3);
    bitmap.set("url", args.at(0).get("url"));
    return bitmap;
}
JSValue failing(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    throw std::runtime_error("decoder refused");
}
}

// The loader keeps the upstream contract around the host boundary: the options record, the cache keyed by the
// resolved url, and the item events. The decoder stands in for fetch plus createImageBitmap.
TEST_CASE("ImageBitmapLoader decodes once and serves the cache") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    decodeCount = 0;
    Cache.clear();          // a previous run may have cached the same url
    Cache.enabled = true;   // the cache is off by default, and the hit path is what the case checks
    ImageBitmapLoader::setDecoder(JSValue::function(record, {}, "decoder", 1));
    ImageBitmapLoader loader;
    const auto first = loader.load("bitmap.png");
    REQUIRE(decodeCount == 1);
    REQUIRE(first.get("width").toNumber() == 2.0);
    REQUIRE(first.get("height").toNumber() == 3.0);
    // A second load comes from the cache keyed by the resolved url.
    const auto second = loader.load("bitmap.png");
    REQUIRE(decodeCount == 1);
    REQUIRE(second.get("width").toNumber() == 2.0);

    // Options start from premultiplyAlpha none, and setOptions replaces the record the decoder receives.
    REQUIRE(lastOption == "none");
    auto replaced = JSValue::object();
    replaced.set("premultiplyAlpha", "premultiply");
    loader.setOptions(replaced);
    loader.load("second.png");       // a fresh url, so the decoder runs again with the new options
    REQUIRE(lastOption == "premultiply");
}

TEST_CASE("ImageBitmapLoader reports a failing decoder") {
    ImageBitmapLoader::setDecoder(JSValue::function(failing, {}, "failing", 1));
    ImageBitmapLoader loader;
    REQUIRE_THROWS_AS(loader.load("broken.png"), std::runtime_error);
    // Without a decoder the loader refuses rather than pretending to decode.
    ImageBitmapLoader::setDecoder(JSValue());
    REQUIRE_THROWS_AS(loader.load("anything.png"), std::exception);
}
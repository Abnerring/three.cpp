#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "constants.h"
#include "core/JSValue.h"
#include "loaders/CompressedTextureLoader.h"
#include "loaders/FileLoader.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

// The base loader's own work is the texData to compressed texture mapping, so these cases drive
// applyTexData through a subclass and pin the upstream shape: the base mip sizes the image, the chain
// becomes the texture's mipmaps, a single mip forces the plain min filter, and the texture is marked for
// upload last. The two per-face forms are refused, and the base parse hook is abstract.
namespace {
class StubLoader : public CompressedTextureLoader {
  public:
    StubLoader() : CompressedTextureLoader() {}
    JSValue payload{};
    JSValue parse(JSValue buffer, JSValue premultiplyAlpha) override {
        static_cast<void>(buffer);
        static_cast<void>(premultiplyAlpha);
        return payload;
    }
};
JSValue texData(const std::string& json) { return Animation_detail::parseJSON(json); }
}

TEST_CASE("CompressedTextureLoader maps a single image with its mipmap chain", "[compressedtextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    StubLoader loader;
    auto texture = std::make_shared<CompressedTexture>();
    loader.applyTexData(texture, texData("{\"width\":64,\"height\":32,\"format\":1023,\"mipmapCount\":3,\"mipmaps\":[{\"width\":64,\"height\":32},{\"width\":32,\"height\":16},{\"width\":16,\"height\":8}]}"));
    REQUIRE(texture->image.get()->width == 64.0);
    REQUIRE(texture->image.get()->height == 32.0);
    REQUIRE(texture->mipmaps->size() == 3);
    REQUIRE(texture->mipmaps->at(1)->width == 32.0);
    REQUIRE(texture->format == 1023.0);
    REQUIRE(texture->version == 1.0);
    // A chain longer than one mip keeps the mipmap min filter upstream's defaults give it.
    REQUIRE(texture->minFilter == LinearMipmapLinearFilter);
}

TEST_CASE("CompressedTextureLoader forces the plain min filter for a single mip", "[compressedtextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    StubLoader loader;
    auto texture = std::make_shared<CompressedTexture>();
    loader.applyTexData(texture, texData("{\"width\":8,\"height\":8,\"format\":1023,\"mipmapCount\":1}"));
    REQUIRE(texture->minFilter == LinearFilter);
    REQUIRE(texture->version == 1.0);
}

TEST_CASE("CompressedTextureLoader refuses the forms it cannot represent", "[compressedtextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    StubLoader loader;
    auto urls = JSValue::array();
    urls.get("push").call(urls, {JSValue("px.dds")});
    // The legacy six-url form attaches a mipmap chain per face, which a native texture cannot hold.
    REQUIRE_THROWS_AS(loader.load(urls), JSTypeError);
    CompressedTextureLoader base;
    REQUIRE_THROWS_AS(base.parse(JSValue{}, JSValue(true)), JSTypeError);
}

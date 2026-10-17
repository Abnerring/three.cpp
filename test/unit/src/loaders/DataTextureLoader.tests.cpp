#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "constants.h"
#include "core/JSValue.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "loaders/DataTextureLoader.h"
#include "loaders/FileLoader.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

// The base loader's own work is the texData to texture mapping, so these cases drive applyTexData
// through a subclass and check every default and every override against the pinned source.
namespace {
// A minimal subclass: the base class has nothing to do in parse, exactly as upstream's abstract hook is
// only useful through a format loader.
class StubLoader : public DataTextureLoader {
  public:
    StubLoader() : DataTextureLoader() {}
    JSValue payload{};
    JSValue parse(JSValue buffer) override {
        static_cast<void>(buffer);
        return payload;
    }
};
JSValue texData(const std::string& json) { return Animation_detail::parseJSON(json); }
std::shared_ptr<DataTexture> applied(const std::string& json) {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto texture = std::make_shared<DataTexture>();
    StubLoader loader;
    loader.applyTexData(texture, texData(json));
    return texture;
}
}

TEST_CASE("DataTextureLoader applies upstream defaults when texData is sparse", "[datatextureloader]") {
    const auto texture = applied("{\"width\":4,\"height\":2,\"data\":null}");
    REQUIRE(texture->wrapS == ClampToEdgeWrapping);
    REQUIRE(texture->wrapT == ClampToEdgeWrapping);
    REQUIRE(texture->magFilter == LinearFilter);
    REQUIRE(texture->minFilter == LinearFilter);
    REQUIRE(texture->anisotropy == 1.0);
    REQUIRE(texture->version == 1.0);          // needsUpdate at the end
    REQUIRE(texture->image.get()->width == 4.0);
    REQUIRE(texture->image.get()->height == 2.0);
}

TEST_CASE("DataTextureLoader applies every field it is given", "[datatextureloader]") {
    const auto texture = applied("{\"width\":8,\"height\":8,\"data\":null,\"wrapS\":1001,\"wrapT\":1000,\"magFilter\":1006,\"minFilter\":1008,\"anisotropy\":4,\"flipY\":true,\"format\":1023,\"type\":1016,\"colorSpace\":\"srgb\",\"generateMipmaps\":true}");
    REQUIRE(texture->wrapS == 1001.0);
    REQUIRE(texture->wrapT == 1000.0);
    REQUIRE(texture->magFilter == 1006.0);
    REQUIRE(texture->minFilter == 1008.0);
    REQUIRE(texture->anisotropy == 4.0);
    REQUIRE(texture->flipY);
    REQUIRE(texture->format == 1023.0);
    REQUIRE(texture->type == 1016.0);
    REQUIRE(texture->colorSpace == "srgb");
    REQUIRE(texture->generateMipmaps);
}

TEST_CASE("DataTextureLoader forces the plain min filter for a single mip", "[datatextureloader]") {
    // Upstream resets the min filter when the texData reports exactly one mip, whatever it asked for.
    const auto texture = applied("{\"width\":1,\"height\":1,\"data\":null,\"minFilter\":1008,\"mipmapCount\":1}");
    REQUIRE(texture->minFilter == LinearFilter);
}

TEST_CASE("DataTextureLoader refuses what it cannot represent", "[datatextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    StubLoader loader;
    auto texture = std::make_shared<DataTexture>();
    REQUIRE_THROWS_AS(loader.applyTexData(texture, texData("{\"mipmaps\":[]}")), JSTypeError);
    DataTextureLoader base;
    REQUIRE_THROWS_AS(base.parse(JSValue{}), JSTypeError);   // the base hook is abstract
}

TEST_CASE("DataTextureLoader.createDataTexture parses then applies", "[datatextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    StubLoader loader;
    loader.payload = texData("{\"width\":2,\"height\":3,\"data\":null,\"flipY\":true}");
    auto typed = std::make_shared<DataTexture>();
    loader.applyTexData(typed, loader.payload);
    // Read the typed fields: the JS wrapper does not expose flipY, which is recorded in the header.
    REQUIRE(typed->flipY);
    REQUIRE(typed->version == 1.0);
    // createDataTexture is the same path behind the JS door, and version is exposed there.
    REQUIRE(loader.createDataTexture(JSValue{}).get("version").toNumber() == 1.0);
}

TEST_CASE("DataTextureLoader loads through the injected transport", "[datatextureloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    // The same dispatch fix applies here: load hands the buffer to the virtual parse on the instance.
    // The transport must be captureless, so it builds its response in place.
    FileLoader::setTransport(JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
        auto headers = JSValue::object();
        headers.set("get", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return JSValue{}; }, {}, "get", 1));
        auto answer = JSValue::object();
        answer.set("status", 200.0);
        answer.set("statusText", "OK");
        answer.set("url", "https://example.com/b.bin");
        answer.set("headers", headers);
        answer.set("body", nativeArray(AnimationArray(Uint8Array(std::vector<std::uint8_t>{9}))));
        return answer;
    }, {}, "transport", 1));
    StubLoader loader;
    loader.payload = texData("{\"width\":2,\"height\":2,\"data\":null,\"flipY\":true}");
    auto surface = loader.load(JSValue("https://example.com/b.bin"));
    REQUIRE(surface.get("version").toNumber() == 1.0);
    FileLoader::setTransport(JSValue{});
}
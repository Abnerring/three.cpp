// Differential probe for the native src/loaders/CompressedTextureLoader.js port.
//
// Each stdin line is one texData document. A stub subclass returns it from parse and the probe drives
// load, so the file transport and the mapping are exercised together: FileLoader's injectable transport
// answers the fetch with canned bytes, the way the comparison script stubs fetch.
//
// What is compared is what the mapping owns end to end - the min filter, the format and the version bump.
// The image dimensions are not read back: their getter throws on a compressed texture without a source,
// so the unit cases cover them from the typed texture instead.
#include "animation/AnimationUtils.h"
#include "constants.h"
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "loaders/CompressedTextureLoader.h"
#include "loaders/FileLoader.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
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
// The transport receives whatever the loader passes and must answer with real bytes, because an
// arraybuffer request converts the body itself.
JSValue transport(const JSValue&, const Args& args, const Args&) {
    const auto request = args.empty() ? JSValue{} : args[0];
    const auto url = request.isObject() ? request.get("url") : request;
    auto headers = JSValue::object();
    headers.set("get", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue{}; }, {}, "get", 1));
    auto response = JSValue::object();
    response.set("status", 200.0);
    response.set("statusText", "OK");
    response.set("url", url);
    response.set("headers", headers);
    response.set("body", nativeArray(AnimationArray(Uint8Array(std::vector<std::uint8_t>{1, 2, 3, 4}))));
    return response;
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    FileLoader::setTransport(JSValue::function(transport, {}, "transport", 1));
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            StubLoader loader;
            loader.payload = Animation_detail::parseJSON(line);
            const auto texture = loader.load(JSValue("https://example.com/data.bin"));
            std::cout << JsonValue(JsonValue::Object{
                {"minFilter", texture.get("minFilter").toNumber()},
                {"format", texture.get("format").toNumber()},
                {"version", texture.get("version").toNumber()}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    FileLoader::setTransport(JSValue{});
    return 0;
}
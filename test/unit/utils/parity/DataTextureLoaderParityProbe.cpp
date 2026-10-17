// Differential probe for the native src/loaders/DataTextureLoader.js port.
//
// Each stdin line is one texData document. A stub subclass returns it from parse, createDataTexture
// applies it, and the probe prints the texture properties the mapping controls. The mipmaps field is not
// exercised here because assigning a mipmap chain is refused natively; the unit tests cover that refusal.
#include "animation/AnimationUtils.h"
#include "constants.h"
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "loaders/DataTextureLoader.h"
#include <iostream>
#include <memory>
#include <string>

namespace {
class StubLoader : public DataTextureLoader {
  public:
    StubLoader() : DataTextureLoader() {}
    JSValue payload{};
    JSValue parse(JSValue buffer) override { static_cast<void>(buffer); return payload; }
};
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            StubLoader loader;
            loader.payload = Animation_detail::parseJSON(line);
            // Read through the JS surface, so the wrapper's property set is compared as well as the
            // mapping.
            const auto texture = loader.createDataTexture(JSValue{});
            std::cout << JsonValue(JsonValue::Object{
                {"wrapS", texture.get("wrapS").toNumber()},
                {"wrapT", texture.get("wrapT").toNumber()},
                {"magFilter", texture.get("magFilter").toNumber()},
                {"minFilter", texture.get("minFilter").toNumber()},
                {"anisotropy", texture.get("anisotropy").toNumber()},
                {"flipY", texture.get("flipY").toBoolean()},
                {"format", texture.get("format").toNumber()},
                {"type", texture.get("type").toNumber()},
                {"colorSpace", texture.get("colorSpace").toString()},
                {"generateMipmaps", texture.get("generateMipmaps").toBoolean()},
                {"version", texture.get("version").toNumber()}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
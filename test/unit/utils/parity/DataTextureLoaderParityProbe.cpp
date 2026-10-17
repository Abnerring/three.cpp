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
            // Read from the typed texture: the JS wrapper does not expose every one of these fields, and
            // the subject here is the mapping, not the wrapper.
            StubLoader loader;
            loader.payload = Animation_detail::parseJSON(line);
            auto texture = std::make_shared<DataTexture>();
            loader.applyTexData(texture, loader.payload);
            std::cout << JsonValue(JsonValue::Object{
                {"wrapS", texture->wrapS},
                {"wrapT", texture->wrapT},
                {"magFilter", texture->magFilter},
                {"minFilter", texture->minFilter},
                {"anisotropy", texture->anisotropy},
                {"flipY", texture->flipY},
                {"format", texture->format},
                {"type", texture->type},
                {"colorSpace", texture->colorSpace},
                {"generateMipmaps", texture->generateMipmaps},
                {"version", texture->version}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
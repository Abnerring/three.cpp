// Differential probe for the native image loading family: src/loaders/ImageLoader.js,
// TextureLoader.js and CubeTextureLoader.js.
//
// Each stdin line is one scenario. `tex <url>` loads a single texture; `cube <n> <url...>` loads a cube
// texture from n urls. The injected decoder synthesises a deterministic image from the url, using the
// same formula the comparison script uses, so both sides feed the loaders identical pixels. What is
// emitted is what the loaders expose - not the pixels themselves, since the wrapper is the subject.
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "loaders/CubeTextureLoader.h"
#include "loaders/ImageLoader.h"
#include "loaders/TextureLoader.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace {
// width = 2 + (h & 3), height = 1 + ((h >> 2) & 3), h = fold(h * 31 + code) over the url.
JSValue decoder(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    const auto request = args.empty() ? JSValue{} : args[0];
    const auto url = request.get("url").toString();
    unsigned hash = 0;
    for (unsigned char code : url) hash = (hash * 31u + code) & 0xffu;
    auto image = JSValue::object();
    image.set("width", static_cast<double>(2 + (hash & 3u)));
    image.set("height", static_cast<double>(1 + ((hash >> 2) & 3u)));
    image.set("url", url);
    return image;
}
JsonValue describe(const JSValue& texture) {
    const auto image = texture.get("image");
    return JsonValue::Object{
        {"width", image.get("width").toNumber()},
        {"height", image.get("height").toNumber()},
        {"hasImage", !image.isUndefined() && !image.isNull()},
        {"version", texture.get("version").toNumber()},
        {"colorSpace", texture.get("colorSpace").toString()}};
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    ImageLoader::setDecoder(JSValue::function(decoder, {}, "decoder", 1));
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::istringstream fields(line);
            std::string mode;
            fields >> mode;
            if (mode == "tex") {
                std::string url;
                fields >> url;
                TextureLoader loader;
                const auto texture = loader.load(JSValue(url));
                std::cout << JsonValue(describe(texture)).dump() << '\n';
            } else if (mode == "cube") {
                int count = 0;
                fields >> count;
                auto urls = JSValue::array();
                for (int index = 0; index < count; ++index) {
                    std::string url;
                    fields >> url;
                    urls.get("push").call(urls, {JSValue(url)});
                }
                CubeTextureLoader loader;
                const auto texture = loader.load(urls);
                // The face count is not reachable through the JS wrapper and CubeTexture has no
                // fromObject, so the cube branch compares what is observable: the colour space and the
                // version, which only moves once six images have arrived. That version is the quirk
                // worth pinning - a shorter array never calls back and so never bumps it.
                std::cout << JsonValue(JsonValue::Object{
                    {"version", texture.get("version").toNumber()},
                    {"colorSpace", texture.get("colorSpace").toString()}}).dump() << '\n';
            } else {
                std::cout << JsonValue(JsonValue::Object{{"probeError", "unknown mode"}}).dump() << '\n';
            }
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
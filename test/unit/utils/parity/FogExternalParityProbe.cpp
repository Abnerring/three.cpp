// Differential probe for the native src/scenes/Fog.js and src/textures/ExternalTexture.js ports.
//
// Each stdin line is `fog color near far` or `external source width height`. Both classes are thin: what is
// compared is the visible state after construction and after a clone, including the defaults that apply when
// the constructor is given nothing.
#include "core/JsonValue.h"
#include "scenes/Fog.h"
#include "textures/ExternalTexture.h"
#include <cmath>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
JsonValue encode(double value) {
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
double at(const std::vector<double>& a, std::size_t index) { return index < a.size() ? a[index] : 0.0; }
JsonValue fogState(const Fog& fog) {
    return JsonValue(JsonValue::Object{{"near", encode(fog.near)}, {"far", encode(fog.far)},
        {"isFog", fog.isFog}, {"red", encode(fog.color.r)}, {"green", encode(fog.color.g)},
        {"blue", encode(fog.color.b)}});
}
JsonValue externalState(const ExternalTexture& texture) {
    return JsonValue(JsonValue::Object{{"isExternalTexture", texture.isExternalTexture},
        {"width", encode(texture.image.get() ? texture.image.get()->width : -1.0)},
        {"height", encode(texture.image.get() ? texture.image.get()->height : -1.0)},
        {"version", encode(texture.version)}});
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        std::vector<double> a;
        std::string token;
        while (fields >> token) a.push_back(std::strtod(token.c_str(), nullptr));
        JsonValue result;
        if (op == "fog") {
            Fog fog(static_cast<int>(at(a, 0)), at(a, 1), at(a, 2));
            result = fogState(fog);
        } else if (op == "fog.clone") {
            Fog fog(static_cast<int>(at(a, 0)), at(a, 1), at(a, 2));
            const auto copy = std::dynamic_pointer_cast<Fog>(fog.clone());
            result = fogState(*copy);
        } else if (op == "external") {
            auto image = std::make_shared<TextureImage>(at(a, 1), at(a, 2));
            const Texture::Image source = image;
            ExternalTexture texture{source};
            result = externalState(texture);
        } else if (op == "external.default") {
            ExternalTexture texture;
            result = externalState(texture);
        } else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
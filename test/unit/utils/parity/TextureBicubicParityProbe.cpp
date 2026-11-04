#include "nodes/accessors/TextureBicubic.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/Texture.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue describe(const JSValue& function, const JSValue& result) {
    auto output = JSValue::object();
    own(output, "name", function.get("name"));
    own(output, "isFn", function.get("isFn"));
    const auto shaderNode = function.get("shaderNode");
    const auto jsFunc = shaderNode.get("jsFunc");
    own(output, "source", jsFunc.get("toString").call(jsFunc));
    own(output, "type", result.get("type"));
    own(output, "nodeType", result.get("node").get("type"));
    return output;
}
}

int main() {
    try {
        const auto texture = nativeTextureConstructor().construct();
        const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.5});
        const auto textureNode = nativeTextureNodeBinding("texture").call({}, {texture, uv});
        const auto lod = nativeTSLBaseBinding("float").call({}, {0.5});
        auto output = JSValue::object();
        own(output, "textureBicubicLevel", describe(nativeTextureBicubicLevel(),
            nativeTextureBicubicLevel().call({}, {textureNode, lod})));
        own(output, "textureBicubic", describe(nativeTextureBicubic(),
            nativeTextureBicubic().call({}, {textureNode, lod})));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "nodes/accessors/Texture3DNode.h"
#include "nodes/tsl/TSLCore.h"
#include "textures/Texture.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue describe(const JSValue& node) {
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "isTexture3DNode", node.get("isTexture3DNode"));
    own(result, "inputType", node.get("getInputType").call(node));
    own(result, "sampler", node.get("getSampler").call(node));
    own(result, "updateMatrix", node.get("updateMatrix"));
    own(result, "uvNodeType", node.get("uvNode").get("type"));
    const auto level = node.get("levelNode");
    own(result, "levelNodeType", level.isNull() || level.isUndefined() ? JSValue(nullptr) : level.get("type"));
    return result;
}
} // namespace

int main() {
    try {
        nativeTexture3DNodeModule();
        const auto texture = nativeTextureConstructor().construct();
        const auto uv = nativeTSLCoreBinding("vec3").call({}, {.25, .5, .75});
        const auto level = nativeTSLCoreBinding("float").call({}, {2});
        const auto texture3D = nativeTexture3DNodeBinding("texture3D");
        const auto node = texture3D.call({}, {texture, uv});
        const auto load = nativeTexture3DNodeBinding("texture3DLoad").call({}, {texture, uv});
        const auto leveled = nativeTexture3DNodeBinding("texture3DLevel").call({}, {texture, uv, level});
        auto result = JSValue::object();
        own(result, "node", describe(node));
        own(result, "load", describe(load));
        own(result, "level", describe(leveled));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

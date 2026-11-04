#include "nodes/accessors/StorageTextureNode.h"
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
    own(result, "isStorageTextureNode", node.get("isStorageTextureNode"));
    own(result, "access", node.get("access"));
    own(result, "mipLevel", node.get("mipLevel"));
    own(result, "uvNodeType", node.get("uvNode").get("type"));
    own(result, "storeNodeType", node.get("storeNode").get("type"));
    own(result, "inputType", node.get("getInputType").call(node));
    return result;
}
}

int main() {
    try {
        nativeStorageTextureNodeModule();
        const auto texture = nativeTextureConstructor().construct();
        const auto uv = nativeTSLCoreBinding("vec2").call({}, {2, 3});
        const auto value = nativeTSLCoreBinding("vec4").call({}, {1, 0, 0, 1});
        const auto node = nativeStorageTextureNodeBinding("storageTexture").call({}, {texture, uv, value});
        auto result = JSValue::object();
        result.set("node", describe(node));
        result.set("readOnly", describe(node.get("toReadOnly").call(node)));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

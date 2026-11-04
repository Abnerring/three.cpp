#include "nodes/accessors/StorageTexture3DNode.h"
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

JSValue describe(const JSValue& node) {
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "isStorageTextureNode", node.get("isStorageTextureNode"));
    own(result, "isStorageTexture3DNode", node.get("isStorageTexture3DNode"));
    own(result, "inputType", node.get("getInputType").call(node));
    own(result, "access", node.get("access"));
    own(result, "mipLevel", node.get("mipLevel"));
    own(result, "updateMatrix", node.get("updateMatrix"));
    own(result, "sampler", node.get("getSampler").call(node));
    own(result, "uvNodeType", node.get("uvNode").get("type"));
    own(result, "storeNodeType", node.get("storeNode").get("type"));
    const auto defaultUV = node.get("getDefaultUV").call(node);
    own(result, "defaultUVType", defaultUV.get("type"));
    return result;
}
} // namespace

int main() {
    try {
        nativeStorageTexture3DNodeModule();
        const auto texture = nativeTextureConstructor().construct();
        const auto uv = nativeTSLBaseBinding("vec3").call({}, {1, 2, 3});
        const auto value = nativeTSLBaseBinding("vec4").call({}, {1, 0, 0, 1});
        const auto node = nativeStorageTexture3DNodeBinding("storageTexture3D").call({}, {texture, uv, value});
        std::cout << describe(node).toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

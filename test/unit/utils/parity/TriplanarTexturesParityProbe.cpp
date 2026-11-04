#include "nodes/utils/TriplanarTextures.h"
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

JSValue describeFn(const JSValue& value) {
    auto result = JSValue::object();
    own(result, "type", value.get("type"));
    own(result, "isFn", value.get("isFn"));
    own(result, "isNode", value.get("isNode"));
    const auto length = value.get("length");
    own(result, "length", length.isCallable() ? JSValue("function") : length);
    own(result, "name", value.get("name"));
    own(result, "source", value.get("shaderNode").get("jsFunc").toString());
    return result;
}

JSValue describeCall(const JSValue& value) {
    auto result = JSValue::object();
    own(result, "type", value.get("type"));
    own(result, "nodeType", value.get("node").get("type"));
    own(result, "rawInputCount", value.get("node").get("rawInputs").get("length"));
    return result;
}
}

int main() {
    try {
        const auto module = nativeTriplanarTexturesModule();
        const auto textures = nativeTriplanarTextures();
        const auto textureAlias = nativeTriplanarTexture();
        const auto textureNode = nativeTextureNodeBinding("texture").call({}, {
            nativeTextureConstructor().construct()});
        const auto vec3 = nativeTSLBaseBinding("vec3");
        const auto floating = nativeTSLBaseBinding("float");
        const auto scale = floating.call({}, {2});
        const auto position = vec3.call({}, {1, 2, 3});
        const auto normal = vec3.call({}, {0, 1, 0});
        const Args callArgs = {textureNode, nullptr, nullptr, scale, position, normal};
        const auto call = textures.call({}, callArgs);
        const auto aliasCall = textureAlias.call({}, callArgs);

        auto output = JSValue::object();
        own(output, "fn", describeFn(textures));
        own(output, "aliasName", textureAlias.get("name"));
        own(output, "aliasLength", textureAlias.get("length"));
        own(output, "moduleSame", module.get("triplanarTextures").sameValue(textures));
        own(output, "aliasModuleSame", module.get("triplanarTexture").sameValue(textureAlias));
        own(output, "call", describeCall(call));
        own(output, "aliasCall", describeCall(aliasCall));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

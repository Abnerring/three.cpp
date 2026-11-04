#include "nodes/accessors/PointUVNode.h"

#include <iostream>

namespace {
using Args = std::vector<JSValue>;

void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue array(const std::vector<JSValue>& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i)
        own(result, std::to_string(i).c_str(), values[i]);
    return result;
}

JSValue call(const JSValue& object, const char* name, const Args& args = {}) {
    return object.get(name).call(object, args);
}
} // namespace

int main() {
    try {
        const auto module = nativePointUVNodeModule();
        const auto constructor = nativePointUVNodeConstructor();
        const auto instance = constructor.construct();
        const auto pointUV = module.get("pointUV");

        auto result = JSValue::object();
        own(result, "exports", array({module.get("default").get("name"), pointUV.get("isNode")}));
        own(result, "constructorName", constructor.get("name"));
        own(result, "constructorLength", constructor.get("length"));
        own(result, "type", constructor.get("type"));
        own(result, "nodeType", instance.get("nodeType"));
        own(result, "isPointUVNode", instance.get("isPointUVNode"));
        own(result, "isCacheable", call(instance, "isCacheable"));
        own(result, "generate", call(instance, "generate", {nullptr}));
        own(result, "pointUVType", pointUV.get("nodeType"));
        own(result, "pointUVIsPointUVNode", pointUV.get("isPointUVNode"));
        own(result, "pointUVCacheable", call(pointUV, "isCacheable"));
        own(result, "pointUVGenerate", call(pointUV, "generate", {nullptr}));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

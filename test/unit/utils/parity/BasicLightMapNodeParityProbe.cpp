#include "nodes/lighting/BasicLightMapNode.h"

#include <iostream>

namespace {

JSValue functionMetadata(const JSValue& function) {
    auto result = JSValue::object();
    result.set("name", function.get("name"));
    result.set("length", function.get("length"));
    result.set("source", function.nativeFunctionSource());
    return result;
}

JSValue names(const JSValue& object) {
    auto result = JSValue::array();
    const auto properties = object.ownPropertyNames();
    for (std::size_t i = 0; i < properties.size(); ++i) result.set(std::to_string(i), properties[i]);
    return result;
}

JSValue metadata() {
    const auto constructor = nativeBasicLightMapNodeConstructor();
    const auto prototype = constructor.get("prototype");
    auto result = JSValue::object();
    result.set("constructor", functionMetadata(constructor));
    result.set("prototypeParent", prototype.getPrototype().get("constructor").get("name"));
    result.set("ownStatic", names(constructor));
    result.set("ownPrototype", names(prototype));
    result.set("setup", functionMetadata(prototype.get("setup")));
    return result;
}

} // namespace

int main() {
    try {
        std::cout << metadata().toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

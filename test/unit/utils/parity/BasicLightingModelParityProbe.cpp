#include "nodes/functions/BasicLightingModel.h"

#include <iostream>
#include <string>

namespace {

void own(JSValue object, const std::string& key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue strings(const std::vector<std::string>& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i) own(result, std::to_string(i), values[i]);
    return result;
}

JSValue functionMetadata(const JSValue& function) {
    auto result = JSValue::object();
    result.set("name", function.get("name"));
    result.set("length", function.get("length"));
    result.set("source", function.nativeFunctionSource());
    return result;
}

JSValue metadata() {
    const auto constructor = nativeBasicLightingModel();
    const auto prototype = constructor.get("prototype");
    auto result = JSValue::object();
    result.set("constructor", functionMetadata(constructor));
    result.set("prototypeParent", prototype.getPrototype().get("constructor").get("name"));
    result.set("ownStatic", strings(constructor.ownPropertyNames()));
    result.set("ownPrototype", strings(prototype.ownPropertyNames()));
    auto methods = JSValue::object();
    methods.set("indirect", functionMetadata(prototype.get("indirect")));
    methods.set("finish", functionMetadata(prototype.get("finish")));
    result.set("methods", methods);
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

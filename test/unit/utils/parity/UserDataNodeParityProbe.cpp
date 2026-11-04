#include "nodes/accessors/UserDataNode.h"

#include <iostream>
#include <string>
#include <vector>

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
        const auto module = nativeUserDataNodeModule();
        const auto constructor = nativeUserDataNodeConstructor();

        auto explicitData = JSValue::object();
        explicitData.set("rotation", 1.5);
        auto fallbackData = JSValue::object();
        fallbackData.set("rotation", 2.5);
        auto object = JSValue::object();
        object.set("userData", fallbackData);
        auto state = JSValue::object();
        state.set("object", object);

        const auto explicitNode = constructor.construct({"rotation", "float", explicitData});
        const auto fallbackNode = constructor.construct({"rotation", "float"});
        const auto explicitReference = call(explicitNode, "updateReference", {state});
        const auto fallbackReference = call(fallbackNode, "updateReference", {state});

        auto result = JSValue::object();
        own(result, "moduleKeys", array({module.get("default").get("name"), module.get("userData").get("name")}));
        own(result, "constructorName", constructor.get("name"));
        own(result, "constructorLength", constructor.get("length"));
        own(result, "type", constructor.get("type"));
        own(result, "explicit", JSValue::object());
        auto explicitResult = result.get("explicit");
        own(explicitResult, "property", explicitNode.get("property"));
        own(explicitResult, "uniformType", explicitNode.get("uniformType"));
        own(explicitResult, "userData", explicitNode.get("userData"));
        own(explicitResult, "reference", explicitReference);
        own(result, "fallback", JSValue::object());
        auto fallbackResult = result.get("fallback");
        own(fallbackResult, "property", fallbackNode.get("property"));
        own(fallbackResult, "uniformType", fallbackNode.get("uniformType"));
        own(fallbackResult, "userData", fallbackNode.get("userData"));
        own(fallbackResult, "reference", fallbackReference);
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

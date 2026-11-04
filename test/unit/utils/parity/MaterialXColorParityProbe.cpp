#include "core/JSJobs.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/materialx/MaterialXColor.h"
#include "nodes/materialx/MaterialXColorTransform.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>
#include <string_view>

namespace {
using Args = std::vector<JSValue>;
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue array(std::initializer_list<JSValue> values) {
    auto result = JSValue::array(); unsigned i = 0;
    for (const auto& value : values) own(result, std::to_string(i++).c_str(), value);
    return result;
}
JSValue metadata(const JSValue& function) {
    auto result = JSValue::object(); result.set("name", function.get("name"));
    result.set("isFn", function.get("isFn")); result.set("layout", function.get("shaderNode").get("layout"));
    result.set("source", function.get("toString").call(function)); return result;
}
}

int main(int argc, char** argv) {
    try {
        nativeTSLBase(); nativeNodeBuilderConstructor(); nativeStackNodeConstructor();
        if (argc == 2 && std::string_view(argv[1]) == "--metadata") {
            auto out = JSValue::object(); out.set("hsvtorgb", metadata(nativeMaterialXColorBinding("mx_hsvtorgb")));
            out.set("rgbtohsv", metadata(nativeMaterialXColorBinding("mx_rgbtohsv")));
            out.set("srgb", metadata(nativeMaterialXColorTransformBinding("mx_srgb_texture_to_lin_rec709")));
            std::cout << out.toJSON()->dump(); return 0;
        }
        const auto previous = nativeTSLBaseBinding("getCurrentStack").call();
        const auto stack = nativeStackNodeConstructor().construct(); nativeTSLBaseBinding("setCurrentStack").call({}, {stack});
        auto vec = nativeTSLBaseBinding("vec3").call({}, {.2, .4, .7});
        auto hsv = nativeMaterialXColorBinding("mx_hsvtorgb").call({}, {vec});
        auto rgb = nativeMaterialXColorBinding("mx_rgbtohsv").call({}, {vec});
        auto srgb = nativeMaterialXColorTransformBinding("mx_srgb_texture_to_lin_rec709").call({}, {vec});
        if (!hsv.get("isNode").toBoolean() || !rgb.get("isNode").toBoolean() || !srgb.get("isNode").toBoolean()) throw std::logic_error("MaterialX color function did not return nodes");
        std::cout << array({hsv.get("type"), rgb.get("type"), srgb.get("type")}).toJSON()->dump();
        nativeTSLBaseBinding("setCurrentStack").call({}, {previous}); return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}


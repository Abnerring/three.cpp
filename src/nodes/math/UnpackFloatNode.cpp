#include "nodes/math/UnpackFloatNode.h"

#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    if (name == "generateNodeType") return self.get("layout").strictEquals("4x8") ? JSValue("vec4") : JSValue("vec2");
    if (name == "generate") {
        const auto builder = arg(args, 0);
        const auto valueNode = self.get("uintNode");
        const auto inputType = invoke(valueNode, "getNodeType")({builder});
        const auto unpacking = invoke(builder, "getFloatUnpackingMethod")({self.get("encoding"), self.get("layout")});
        const auto value = invoke(valueNode, "build")({builder, inputType});
        return unpacking.toString() + "(" + value.toString() + ")";
    }
    throw std::logic_error("Unknown UnpackFloatNode method");
}

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue makeFactory(const JSValue& constructor, const char* encoding, const JSValue& settings = {}) {
    auto proxy = nativeTSLCoreBinding("nodeProxyIntent").call({}, {constructor, encoding, nullptr, settings});
    return invoke(proxy, "setParameterLength")({1});
}
} // namespace

JSValue nativeUnpackFloatNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    auto constructor = makeConstructor(
        "UnpackFloatNode", 2,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("encoding", arg(args, 0));
            self.set("uintNode", arg(args, 1));
            self.set("layout", "2x16");
            self.set("isUnpackFloatNode", true);
            return self;
        },
        {{"generateNodeType", 0}, {"generate", 1}}, method);
    weak = JSWeakValue(constructor);
    return constructor;
}

JSValue nativeUnpackFloatNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "UnpackFloatNode module");
    const auto constructor = nativeUnpackFloatNodeConstructor();
    auto settings = JSValue::object();
    settings.set("layout", "4x8");
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "unpackSnorm2x16", makeFactory(constructor, "snorm"));
    own(bindings, "unpackUnorm2x16", makeFactory(constructor, "unorm"));
    own(bindings, "unpackHalf2x16", makeFactory(constructor, "float16"));
    own(bindings, "unpackSnorm4x8", makeFactory(constructor, "snorm", settings));
    own(bindings, "unpackUnorm4x8", makeFactory(constructor, "unorm", settings));
    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeUnpackFloatNodeBinding(const std::string& name) {
    const auto keeper = nativeUnpackFloatNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

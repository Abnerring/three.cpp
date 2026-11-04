#include "nodes/math/PackFloatNode.h"

#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    const auto builder = arg(args, 0);
    if (name == "generateNodeType") return "uint";
    if (name == "generate") {
        const auto vectorNode = self.get("vectorNode");
        const auto inputType = invoke(vectorNode, "getNodeType")({builder});
        const auto packing = invoke(builder, "getFloatPackingMethod")({self.get("encoding"), self.get("layout")});
        const auto value = invoke(vectorNode, "build")({builder, inputType});
        return packing.toString() + "(" + value.toString() + ")";
    }
    throw std::logic_error("Unknown PackFloatNode method");
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

JSValue nativePackFloatNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;

    auto constructor = makeConstructor(
        "PackFloatNode", 2,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("encoding", arg(args, 0));
            self.set("vectorNode", arg(args, 1));
            self.set("layout", "2x16");
            self.set("isPackFloatNode", true);
            return self;
        },
        {{"generateNodeType", 0}, {"generate", 1}}, method);
    weak = JSWeakValue(constructor);
    return constructor;
}

JSValue nativePackFloatNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PackFloatNode module");
    const auto constructor = nativePackFloatNodeConstructor();
    auto settings = JSValue::object();
    settings.set("layout", "4x8");
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "packSnorm2x16", makeFactory(constructor, "snorm"));
    own(bindings, "packUnorm2x16", makeFactory(constructor, "unorm"));
    own(bindings, "packHalf2x16", makeFactory(constructor, "float16"));
    own(bindings, "packSnorm4x8", makeFactory(constructor, "snorm", settings));
    own(bindings, "packUnorm4x8", makeFactory(constructor, "unorm", settings));

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativePackFloatNodeBinding(const std::string& name) {
    const auto keeper = nativePackFloatNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

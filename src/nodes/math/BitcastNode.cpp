#include "nodes/math/BitcastNode.h"

#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <tuple>

namespace {
using namespace UtilityNodeRuntime;

JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    const auto builder = arg(args, 0);

    if (name == "generateNodeType") {
        const auto inputType = self.get("inputType");
        if (!inputType.isNull()) {
            const auto valueType = invoke(self.get("valueNode"), "getNodeType")({builder});
            const auto valueLength = invoke(builder, "getTypeLength")({valueType});
            return invoke(builder, "getTypeFromLength")({valueLength, inputType});
        }
        return self.get("conversionType");
    }

    if (name == "generate") {
        const auto valueNode = self.get("valueNode");
        const auto type = invoke(self, "getNodeType")({builder});
        JSValue inputType;
        const auto requestedInputType = self.get("inputType");
        if (!requestedInputType.isNull()) {
            const auto valueType = invoke(valueNode, "getNodeType")({builder});
            const auto valueLength = invoke(builder, "getTypeLength")({valueType});
            inputType = valueLength.strictEquals(1)
                ? requestedInputType
                : invoke(builder, "changeComponentType")({valueType, requestedInputType});
        } else {
            inputType = invoke(valueNode, "getNodeType")({builder});
        }

        const auto bitcastMethod = invoke(builder, "getBitcastMethod")({type, inputType});
        const auto snippet = invoke(valueNode, "build")({builder, inputType});
        return bitcastMethod.toString() + "( " + snippet.toString() + " )";
    }

    throw std::logic_error("Unknown BitcastNode method");
}

JSValue directFactory(const JSValue&, const Args& args, const Args& captures) {
    return captures[0].construct({arg(args, 0), captures[1], captures[2]});
}

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}
} // namespace

JSValue nativeBitcastNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;

    auto constructor = makeConstructor(
        "BitcastNode", 2,
        +[](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(newTarget, captures);
            self.set("valueNode", arg(args, 0));
            self.set("conversionType", arg(args, 1));
            self.set("inputType", arg(args, 2, nullptr));
            self.set("isBitcastNode", true);
            return self;
        },
        {{"generateNodeType", 1}, {"generate", 1}}, method);

    weak = JSWeakValue(constructor);
    return constructor;
}

JSValue nativeBitcastNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BitcastNode exports");
    const auto tsl = nativeTSLCore();
    (void)tsl;
    const auto constructor = nativeBitcastNodeConstructor();
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);

    auto bitcast = nativeTSLCoreBinding("nodeProxyIntent").call({}, {constructor});
    bitcast = invoke(bitcast, "setParameterLength")({2});
    own(bindings, "bitcast", bitcast);

    for (const auto& [name, conversion, input] : std::initializer_list<std::tuple<const char*, const char*, const char*>>{
             {"floatBitsToInt", "int", "float"},
             {"floatBitsToUint", "uint", "float"},
             {"intBitsToFloat", "float", "int"},
             {"uintBitsToFloat", "float", "uint"}}) {
        own(bindings, name, JSValue::function(directFactory,
                                              {constructor, conversion, input}, name, 1));
    }

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeBitcastNodeBinding(const std::string& name) {
    const auto keeper = nativeBitcastNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

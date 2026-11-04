#include "nodes/math/Packed4x8IntegerNode.h"

#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

constexpr const char* DOT4_U8_PACKED = "dot4U8Packed";
constexpr const char* DOT4_I8_PACKED = "dot4I8Packed";
constexpr const char* PACK4X_I8 = "pack4xI8";
constexpr const char* PACK4X_U8 = "pack4xU8";
constexpr const char* PACK4X_I8_CLAMP = "pack4xI8Clamp";
constexpr const char* PACK4X_U8_CLAMP = "pack4xU8Clamp";
constexpr const char* UNPACK4X_I8 = "unpack4xI8";
constexpr const char* UNPACK4X_U8 = "unpack4xU8";

JSValue typed(const char* name, const JSValue& value) {
    return nativeTSLBaseBinding(name).call({}, {value});
}

JSValue packU8(const JSValue& value) {
    const auto uint = nativeTSLBaseBinding("uint");
    const auto mask = uint.call({}, {255});
    const auto x = invoke(value.get("x"), "bitAnd")({mask});
    const auto y = invoke(value.get("y"), "bitAnd")({mask});
    const auto z = invoke(value.get("z"), "bitAnd")({mask});
    const auto w = invoke(value.get("w"), "bitAnd")({mask});
    auto result = invoke(x, "bitOr")({invoke(y, "shiftLeft")({uint.call({}, {8})})});
    result = invoke(result, "bitOr")({invoke(z, "shiftLeft")({uint.call({}, {16})})});
    return invoke(result, "bitOr")({invoke(w, "shiftLeft")({uint.call({}, {24})})});
}

JSValue unpackU8(const JSValue& value) {
    const auto uint = nativeTSLBaseBinding("uint");
    const auto mask = uint.call({}, {255});
    const auto x = invoke(value, "bitAnd")({mask});
    const auto y = invoke(invoke(value, "shiftRight")({uint.call({}, {8})}), "bitAnd")({mask});
    const auto z = invoke(invoke(value, "shiftRight")({uint.call({}, {16})}), "bitAnd")({mask});
    const auto w = invoke(invoke(value, "shiftRight")({uint.call({}, {24})}), "bitAnd")({mask});
    return nativeTSLBaseBinding("uvec4").call({}, {x, y, z, w});
}

JSValue signExtend(const JSValue& value, unsigned shift) {
    const auto uint = nativeTSLBaseBinding("uint");
    const auto shifted = invoke(value, "shiftLeft")({uint.call({}, {static_cast<double>(24 - shift)})});
    return invoke(typed("int", shifted), "shiftRight")({24});
}

JSValue unpackI8(const JSValue& value) {
    return nativeTSLBaseBinding("ivec4").call({}, {
        signExtend(value, 0), signExtend(value, 8), signExtend(value, 16), signExtend(value, 24)});
}

JSValue emulatedBody(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto method = captures[0].string();
    const auto a = inputs.get("a").isUndefined() ? inputs.get("v") : inputs.get("a");
    const auto b = inputs.get("b");
    if (method == PACK4X_U8) return packU8(a);
    if (method == PACK4X_I8) return packU8(typed("uvec4", a));
    if (method == PACK4X_U8_CLAMP) {
        const auto type = nativeTSLBaseBinding("uvec4");
        return packU8(invoke(a, "clamp")({type.call({}, {0}), type.call({}, {255})}));
    }
    if (method == PACK4X_I8_CLAMP) {
        const auto type = nativeTSLBaseBinding("ivec4");
        return packU8(typed("uvec4", invoke(a, "clamp")({type.call({}, {-128}), type.call({}, {127})})));
    }
    if (method == UNPACK4X_U8) return unpackU8(a);
    if (method == UNPACK4X_I8) return unpackI8(a);
    const auto ua = method == DOT4_I8_PACKED ? unpackI8(a) : unpackU8(a);
    const auto ub = method == DOT4_I8_PACKED ? unpackI8(b) : unpackU8(b);
    auto result = invoke(ua.get("x"), "mul")({ub.get("x")});
    result = invoke(result, "add")({invoke(ua.get("y"), "mul")({ub.get("y")})});
    result = invoke(result, "add")({invoke(ua.get("z"), "mul")({ub.get("z")})});
    return invoke(result, "add")({invoke(ua.get("w"), "mul")({ub.get("w")})});
}

JSValue emulatedFunction(const std::string& method) {
    auto callback = JSValue::function(emulatedBody, {method}, "", 1);
    JSValue layout = JSValue::object();
    if (method == DOT4_U8_PACKED || method == DOT4_I8_PACKED) {
        own(layout, "a", "uint"); own(layout, "b", "uint");
        own(layout, "return", method == DOT4_I8_PACKED ? "int" : "uint");
    } else if (method == PACK4X_I8 || method == PACK4X_I8_CLAMP) {
        own(layout, "v", "ivec4"); own(layout, "return", "uint");
    } else if (method == PACK4X_U8 || method == PACK4X_U8_CLAMP) {
        own(layout, "v", "uvec4"); own(layout, "return", "uint");
    } else {
        own(layout, "v", "uint"); own(layout, "return", method == UNPACK4X_I8 ? "ivec4" : "uvec4");
    }
    return nativeTSLBaseBinding("Fn").call({}, {callback, layout});
}

JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    const auto methodName = self.get("method").string();
    if (name == "getInputType") {
        if (methodName == PACK4X_I8 || methodName == PACK4X_I8_CLAMP) return "ivec4";
        if (methodName == PACK4X_U8 || methodName == PACK4X_U8_CLAMP) return "uvec4";
        return "uint";
    }
    if (name == "generateNodeType") {
        if (methodName == DOT4_I8_PACKED) return "int";
        if (methodName == UNPACK4X_I8) return "ivec4";
        if (methodName == UNPACK4X_U8) return "uvec4";
        return "uint";
    }
    if (name == "setup") {
        const auto fn = emulatedFunction(methodName);
        Args parameters{self.get("aNode")};
        if (!self.get("bNode").isNull()) parameters.push_back(self.get("bNode"));
        return fn.call({}, parameters);
    }
    if (name == "generate") {
        const auto builder = arg(args, 0);
        const auto output = arg(args, 1);
        const auto inputType = invoke(self, "getInputType")();
        auto code = invoke(self.get("aNode"), "build")({builder, inputType}).toString();
        if (!self.get("bNode").isNull()) code += ", " + invoke(self.get("bNode"), "build")({builder, inputType}).toString();
        const auto type = invoke(self, "getNodeType")({builder});
        return invoke(builder, "format")({JSValue(methodName + "( " + code + " )"), type, output});
    }
    if (name == "serialize") {
        auto data = arg(args, 0);
        superMethod(self, "serialize", captures, {data});
        data.set("method", self.get("method"));
        return data;
    }
    if (name == "deserialize") {
        auto data = arg(args, 0);
        superMethod(self, "deserialize", captures, {data});
        auto result = self;
        result.set("method", data.get("method"));
        return result;
    }
    throw std::logic_error("Unknown Packed4x8IntegerNode method");
}

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue makeFactory(const JSValue& constructor, const char* methodName) {
    auto proxy = nativeTSLCoreBinding("nodeProxyIntent").call({}, {constructor, methodName});
    return invoke(proxy, "setParameterLength")({std::string(methodName).find("dot4") == 0 ? 2 : 1});
}
} // namespace

JSValue nativePacked4x8IntegerNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    auto constructor = makeConstructor(
        "Packed4x8IntegerNode", 3,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("method", arg(args, 0)); self.set("aNode", arg(args, 1));
            self.set("bNode", arg(args, 2, nullptr)); self.set("isPacked4x8IntegerNode", true);
            return self;
        },
        {{"getInputType", 0}, {"generateNodeType", 0}, {"setup", 1}, {"generate", 2},
         {"serialize", 1}, {"deserialize", 1}}, method);
    constructor.set("DOT4_U8_PACKED", DOT4_U8_PACKED);
    constructor.set("DOT4_I8_PACKED", DOT4_I8_PACKED);
    constructor.set("PACK4X_I8", PACK4X_I8);
    constructor.set("PACK4X_U8", PACK4X_U8);
    constructor.set("PACK4X_I8_CLAMP", PACK4X_I8_CLAMP);
    constructor.set("PACK4X_U8_CLAMP", PACK4X_U8_CLAMP);
    constructor.set("UNPACK4X_I8", UNPACK4X_I8);
    constructor.set("UNPACK4X_U8", UNPACK4X_U8);
    weak = JSWeakValue(constructor); return constructor;
}

JSValue nativePacked4x8IntegerNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Packed4x8IntegerNode module");
    const auto constructor = nativePacked4x8IntegerNodeConstructor();
    auto bindings = JSValue::object(nullptr); own(bindings, "default", constructor);
    for (const auto& name : {DOT4_U8_PACKED, DOT4_I8_PACKED, PACK4X_I8, PACK4X_U8,
                             PACK4X_I8_CLAMP, PACK4X_U8_CLAMP, UNPACK4X_I8, UNPACK4X_U8})
        own(bindings, name, makeFactory(constructor, name));
    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime(); lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}

JSValue nativePacked4x8IntegerNodeBinding(const std::string& name) {
    const auto keeper = nativePacked4x8IntegerNodeModule(); (void)keeper;
    return lexical().lock()->get(name);
}

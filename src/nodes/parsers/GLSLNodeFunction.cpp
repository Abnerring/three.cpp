#include "nodes/parsers/GLSLNodeFunction.h"
#include "nodes/core/NodeFunction.h"
#include "nodes/core/NodeFunctionInput.h"
#include "core/JSRegExp.h"
#include "core/JSModuleObject.h"
#include "core/JSStringEncoding.h"
#include <cmath>
#include <cstdlib>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) return a.toString() + b.toString();
    return a.toNumber() + b.toNumber();
}
// Number.parseInt with omitted radix. Normally the token is an ASCII word;
// exec overrides can also supply arbitrary values, whitespace and signs.
double tokenCount(const JSValue& value) {
    auto units = jsUTF16(value.toString()); std::size_t start = 0;
    const auto whitespace = [](char16_t c) { return (c >= 0x09 && c <= 0x0d) || c == 0x20 || c == 0xa0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000 || c == 0xfeff; };
    while (start < units.size() && whitespace(units[start])) ++start;
    bool negative = false; if (start < units.size() && (units[start] == '-' || units[start] == '+')) { negative = units[start] == '-'; ++start; }
    const auto text = jsUTF8(std::u16string_view(units).substr(start)); std::size_t end = 0;
    if (text.size() > 1 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        end = 2; while (end < text.size() && ((text[end] >= '0' && text[end] <= '9') || (text[end] >= 'a' && text[end] <= 'f') || (text[end] >= 'A' && text[end] <= 'F'))) ++end;
        if (end == 2) return std::numeric_limits<double>::quiet_NaN();
        const auto parsed = std::strtod((text.substr(0, end) + "p0").c_str(), nullptr); return negative ? -parsed : parsed;
    }
    while (end < text.size() && text[end] >= '0' && text[end] <= '9') ++end;
    if (end == 0) return std::numeric_limits<double>::quiet_NaN();
    const auto parsed = std::strtod(text.substr(0, end).c_str(), nullptr); return negative ? -parsed : parsed;
}
JSValue parse(JSValue source, const Args& captures) {
    source = invoke(source, "trim")();
    const auto pragmaIndex = invoke(source, "indexOf")({"#pragma main"});
    const auto mainSource = !pragmaIndex.strictEquals(-1) ? invoke(source, "slice")({add(pragmaIndex, 12)}) : source;
    const auto mainCode = invoke(mainSource, "replace")({createNativeRegExp(R"(^(?:\s*\/\/[^\r\n]*|\s*\/\*[\s\S]*?\*\/|\s*)+)"), ""});
    const auto declaration = invoke(mainCode, "match")({captures[1]});
    if (declaration.isNull() || !declaration.get("length").strictEquals(5)) throw std::runtime_error("THREE.FunctionNode: Function is not a GLSL code.");
    const auto inputsCode = declaration.get("4"); auto matches = JSValue::array();
    for (;;) { const auto match = invoke(captures[2], "exec")({inputsCode}); if (match.isNull()) break; invoke(matches, "push")({match}); }
    auto inputs = JSValue::array(); std::uint32_t i = 0;
    while (i < matches.get("length").toNumber()) {
        const auto isConst = matches.get(std::to_string(i)).get("0").strictEquals("const"); if (isConst) ++i;
        auto qualifier = matches.get(std::to_string(i)).get("0");
        if (qualifier.strictEquals("in") || qualifier.strictEquals("out") || qualifier.strictEquals("inout")) ++i; else qualifier = "";
        const auto type = matches.get(std::to_string(i++)).get("0");
        const auto parsedCount = tokenCount(matches.get(std::to_string(i)).get("0"));
        JSValue count; if (!std::isnan(parsedCount)) { ++i; count = parsedCount; } else count = nullptr;
        const auto name = matches.get(std::to_string(i++)).get("0");
        invoke(inputs, "push")({captures[3].construct({type, name, count, qualifier, isConst})});
    }
    const auto blockCode = invoke(mainCode, "substring")({declaration.get("0").get("length")});
    const auto name = declaration.get("3").isUndefined() ? JSValue("") : declaration.get("3");
    const auto type = declaration.get("2"); const auto precision = declaration.get("1").isUndefined() ? JSValue("") : declaration.get("1");
    const auto headerCode = !pragmaIndex.strictEquals(-1) ? invoke(source, "slice")({0, pragmaIndex}) : JSValue("");
    auto result = JSValue::object(nullptr); result.set("type", type); result.set("inputs", inputs); result.set("name", name); result.set("precision", precision);
    result.set("inputsCode", inputsCode); result.set("blockCode", blockCode); result.set("headerCode", headerCode); return result;
}
}
JSValue nativeGLSLNodeFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto live = weak.lock(); live && live->isConstructible()) return *live;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "GLSLNodeFunction");
    const auto base = nativeNodeFunctionConstructor(); auto holder = JSValue::object(nullptr);
    const auto declaration = createNativeRegExp(R"(^\s*(highp|mediump|lowp)?\s*([a-z_0-9]+)\s*([a-z_0-9]+)?\s*\(([\s\S]*?)\))", "i");
    const auto properties = createNativeRegExp("[a-z_0-9]+", "ig"), input = nativeNodeFunctionInputConstructor();
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        const auto parsed = parse(args.empty() ? JSValue{} : args[0], captures);
        auto self = captures[0].get("constructor").getPrototype().construct({parsed.get("type"), parsed.get("inputs"), parsed.get("name"), parsed.get("precision")}, newTarget);
        self.set("inputsCode", parsed.get("inputsCode")); self.set("blockCode", parsed.get("blockCode")); self.set("headerCode", parsed.get("headerCode")); return self;
    }, {holder, declaration, properties, input}, "GLSLNodeFunction", 1);
    holder.set("constructor", constructor); constructor.setPrototype(base); auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor method; method.value = JSValue::function([](const JSValue& self, const Args& args, const Args&) -> JSValue {
        const auto name = args.empty() || args[0].isUndefined() ? self.get("name") : args[0];
        const auto block = self.get("blockCode");
        if (block.strictEquals("")) return "";
        const auto type = self.get("type"), inputs = self.get("inputsCode"), header = self.get("headerCode"), precision = self.get("precision");
        auto declaration = type.toString(); declaration += " "; declaration += name.toString(); declaration += " ( ";
        declaration += invoke(inputs, "trim")().toString(); declaration += " )";
        if (!precision.strictEquals("")) declaration = precision.toString() + " " + declaration;
        return add(add(header, declaration), block);
    }, {}, "getCode", 0); method.writable = method.configurable = true; prototype.defineProperty("getCode", method);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
GLSLNodeFunction::GLSLNodeFunction(const JSValue& source) : object_(constructor().construct({source})) {}
JSValue GLSLNodeFunction::getCode(const JSValue& name) const { return object_.get("getCode").call(object_, {name}); }

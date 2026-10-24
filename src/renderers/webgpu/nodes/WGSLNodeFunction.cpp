#include "renderers/webgpu/nodes/WGSLNodeFunction.h"
#include "nodes/core/NodeFunction.h"
#include "nodes/core/NodeFunctionInput.h"
#include "core/JSRegExp.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) return a.toString() + b.toString();
    return a.toNumber() + b.toNumber();
}
void field(JSValue object, const char* name, const JSValue& value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JSValue types() {
    auto out = JSValue::object();
    for (const auto& [key, value] : std::initializer_list<std::pair<const char*, const char*>>{
        {"f32","float"},{"i32","int"},{"u32","uint"},{"bool","bool"},
        {"vec2<f32>","vec2"},{"vec2<i32>","ivec2"},{"vec2<u32>","uvec2"},{"vec2<bool>","bvec2"},
        {"vec2f","vec2"},{"vec2i","ivec2"},{"vec2u","uvec2"},{"vec2b","bvec2"},
        {"vec3<f32>","vec3"},{"vec3<i32>","ivec3"},{"vec3<u32>","uvec3"},{"vec3<bool>","bvec3"},
        {"vec3f","vec3"},{"vec3i","ivec3"},{"vec3u","uvec3"},{"vec3b","bvec3"},
        {"vec4<f32>","vec4"},{"vec4<i32>","ivec4"},{"vec4<u32>","uvec4"},{"vec4<bool>","bvec4"},
        {"vec4f","vec4"},{"vec4i","ivec4"},{"vec4u","uvec4"},{"vec4b","bvec4"},
        {"mat2x2<f32>","mat2"},{"mat2x2f","mat2"},{"mat3x3<f32>","mat3"},{"mat3x3f","mat3"},{"mat4x4<f32>","mat4"},{"mat4x4f","mat4"},
        {"sampler","sampler"},{"sampler_comparison","samplerComparison"},{"texture_1d","texture"},
        {"texture_2d","texture"},{"texture_2d_array","texture"},{"texture_multisampled_2d","cubeTexture"},
        {"texture_depth_2d","depthTexture"},{"texture_depth_2d_array","depthTexture"},{"texture_depth_multisampled_2d","depthTexture"},
        {"texture_depth_cube","depthTexture"},{"texture_depth_cube_array","depthTexture"},{"texture_3d","texture3D"},
        {"texture_cube","cubeTexture"},{"texture_cube_array","cubeTexture"},{"texture_storage_1d","storageTexture"},
        {"texture_storage_2d","storageTexture"},{"texture_storage_2d_array","storageTexture"},{"texture_storage_3d","storageTexture"}}) field(out, key, value);
    return out;
}
JSValue parse(JSValue source, const Args& captures) {
    source = invoke(source, "replace")({createNativeRegExp(R"(^(?:\s*\/\/[^\r\n]*|\s*\/\*[\s\S]*?\*\/|\s*)+)"), ""});
    const auto declaration = invoke(source, "match")({captures[1]});
    if (declaration.isNull() || !declaration.get("length").strictEquals(4)) throw std::runtime_error("THREE.WGSLNodeFunction: Function is not a WGSL code.");
    const auto inputsCode = declaration.get("2"); auto matches = JSValue::array();
    for (;;) {
        const auto match = invoke(captures[2], "exec")({inputsCode}); if (match.isNull()) break;
        const auto push = invoke(matches, "push");
        auto item = JSValue::object(); field(item, "name", match.get("1")); field(item, "type", match.get("2")); push({item});
    }
    auto inputs = JSValue::array();
    for (std::uint32_t i = 0; i < matches.get("length").toNumber(); ++i) {
        const auto item = matches.get(std::to_string(i)), name = item.get("name"), type = item.get("type"); auto resolved = type;
        if (invoke(resolved, "startsWith")({"ptr"}).toBoolean()) resolved = "pointer";
        else {
            if (invoke(resolved, "startsWith")({"texture"}).toBoolean()) resolved = invoke(type, "split")({"<"}).get("0");
            resolved = captures[4].get(resolved);
        }
        invoke(inputs, "push")({captures[3].construct({resolved, name})});
    }
    const auto block = invoke(source, "substring")({declaration.get("0").get("length")});
    auto output = declaration.get("3"); if (!output.toBoolean()) output = "void";
    const auto name = declaration.get("1").isUndefined() ? JSValue("") : declaration.get("1"); auto type = captures[4].get(output); if (!type.toBoolean()) type = output;
    auto result = JSValue::object(nullptr); result.set("type", type); result.set("inputs", inputs); result.set("name", name);
    result.set("inputsCode", inputsCode); result.set("blockCode", block); result.set("outputType", output); return result;
}
}
JSValue nativeWGSLNodeFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto live = weak.lock(); live && live->isConstructible()) return *live;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "WGSLNodeFunction");
    const auto base = nativeNodeFunctionConstructor(); auto holder = JSValue::object(nullptr);
    const auto declaration = createNativeRegExp(R"(^[fn]*\s*([a-z_0-9]+)?\s*\(([\s\S]*?)\)\s*[\-\>]*\s*([a-z_0-9]+(?:<[\s\S]+?>)?))", "i");
    const auto properties = createNativeRegExp(R"(([a-z_0-9]+)\s*:\s*([a-z_0-9]+(?:<[\s\S]+?>)?))", "ig");
    const auto input = nativeNodeFunctionInputConstructor(), library = types();
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        const auto parsed = parse(args.empty() ? JSValue{} : args[0], captures);
        auto self = captures[0].get("constructor").getPrototype().construct({parsed.get("type"), parsed.get("inputs"), parsed.get("name")}, newTarget);
        self.set("inputsCode", parsed.get("inputsCode")); self.set("blockCode", parsed.get("blockCode")); self.set("outputType", parsed.get("outputType")); return self;
    }, {holder, declaration, properties, input, library}, "WGSLNodeFunction", 1);
    holder.set("constructor", constructor); constructor.setPrototype(base); auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor method; method.value = JSValue::function([](const JSValue& self, const Args& args, const Args&) -> JSValue {
        const auto name = args.empty() || args[0].isUndefined() ? self.get("name") : args[0];
        const auto output = !self.get("outputType").strictEquals("void") ? add("-> ", self.get("outputType")) : JSValue("");
        auto prefix = std::string("fn ") + name.toString(); prefix += " ( ";
        prefix += invoke(self.get("inputsCode"), "trim")().toString(); prefix += " ) "; prefix += output.toString();
        return add(prefix, self.get("blockCode"));
    }, {}, "getCode", 0); method.writable = method.configurable = true; prototype.defineProperty("getCode", method);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
WGSLNodeFunction::WGSLNodeFunction(const JSValue& source) : object_(constructor().construct({source})) {}
JSValue WGSLNodeFunction::getCode(const JSValue& name) const { return object_.get("getCode").call(object_, {name}); }

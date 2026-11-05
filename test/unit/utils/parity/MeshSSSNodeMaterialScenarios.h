#pragma once

#include "materials/nodes/MeshSSSNodeMaterial.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"

#include <string>
#include <vector>

namespace MeshSSSNodeMaterialScenarios {
using Args = std::vector<JSValue>;
inline JSValue object() { return JSValue::object(nullptr); }
inline void push(JSValue array, const JSValue& value) { array.set(array.length(), value); }
inline JSValue call(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
inline JSValue node(const char* type, const Args& args) { return nativeTSLBaseBinding(type).call({}, args); }
inline JSValue names(const std::vector<std::string>& values) {
    auto out = JSValue::array();
    for (const auto& value : values) push(out, value);
    return out;
}
inline JSValue graph(const JSValue& value) {
    if (value.sameValue(nativeNormalBinding("normalView"))) return "@normalView";
    if (value.sameValue(nativePositionBinding("positionViewDirection"))) return "@positionViewDirection";
    if (value.isUndefined()) return "@undefined";
    if (!value.isObject()) return value;
    if (value.type() == JSValue::Type::Array) {
        auto out = JSValue::array();
        for (unsigned i = 0; i < value.length(); ++i) push(out, graph(value.get(JSValue(i))));
        return out;
    }
    auto out = object();
    if (value.get("isNode").strictEquals(true)) {
        out.set("type", value.get("type"));
        for (const auto* key : {"nodeType", "value", "method", "op", "aNode", "bNode", "cNode",
                                "node", "nodes", "convertTo", "components", "property", "global"})
            if (value.getOwnPropertyDescriptor(key)) out.set(key, graph(value.get(key)));
    } else {
        for (const auto& key : value.keys()) out.set(key, graph(value.get(key)));
    }
    return out;
}
inline JSValue defaults(const JSValue& material) {
    auto out = object();
    out.set("thicknessColorNode", graph(material.get("thicknessColorNode")));
    for (const auto* key : {"thicknessDistortionNode", "thicknessAmbientNode", "thicknessAttenuationNode",
                            "thicknessPowerNode", "thicknessScaleNode"}) out.set(key, graph(material.get(key)));
    return out;
}
inline JSValue metadata() {
    const auto ctor = nativeMeshSSSNodeMaterialConstructor(), material = ctor.construct();
    const auto prototype = ctor.get("prototype"), model = call(material, "setupLightingModel");
    const auto modelCtor = model.get("constructor"), modelPrototype = modelCtor.get("prototype");
    auto out = object();
    out.set("name", ctor.get("name")); out.set("length", ctor.get("length")); out.set("type", ctor.get("type"));
    out.set("parent", ctor.getPrototype().get("name")); out.set("materialType", material.get("type"));
    out.set("ownFlag", material.getOwnPropertyDescriptor("isMeshSSSNodeMaterial").has_value());
    out.set("prototypeNames", names(prototype.ownPropertyNames()));
    out.set("source", call(ctor, "toString"));
    out.set("typeSource", call(*ctor.getOwnPropertyDescriptor("type")->getter, "toString"));
    out.set("useSSSSource", call(*prototype.getOwnPropertyDescriptor("useSSS")->getter, "toString"));
    out.set("setupSource", call(prototype.get("setupLightingModel"), "toString"));
    out.set("modelName", modelCtor.get("name")); out.set("modelLength", modelCtor.get("length"));
    out.set("modelParent", modelCtor.getPrototype().get("name"));
    out.set("modelPrototypeNames", names(modelPrototype.ownPropertyNames()));
    out.set("modelSource", call(modelCtor, "toString"));
    out.set("directSource", call(modelPrototype.get("direct"), "toString"));
    out.set("directLength", modelPrototype.get("direct").get("length"));
    out.set("defaults", defaults(material));
    return out;
}
inline JSValue constructorAndCopy() {
    const auto ctor = nativeMeshSSSNodeMaterialConstructor();
    auto parameters = object();
    parameters.set("roughness", 0.25); parameters.set("thickness", 2.0);
    for (const auto* key : {"clearcoat", "sheen", "iridescence", "anisotropy", "transmission", "dispersion",
                            "retroreflectivity", "diffuseRoughness"}) parameters.set(key, 1.0);
    auto material = ctor.construct({parameters});
    auto out = object();
    out.set("roughness", material.get("roughness")); out.set("thickness", material.get("thickness"));
    out.set("defaults", defaults(material));
    out.set("initialSSS", material.get("useSSS"));
    const auto color = node("vec3", {0.8, 0.4, 0.2}), power = node("float", {7.0});
    auto enabled = JSValue::array();
    for (const auto& value : Args{nullptr, false, 0, JSValue{}, color}) {
        material.set("thicknessColorNode", value); push(enabled, material.get("useSSS"));
    }
    out.set("enabled", enabled);
    material.set("thicknessPowerNode", power);
    const auto model = call(material, "setupLightingModel");
    auto flags = object();
    for (const auto* key : {"clearcoat", "sheen", "iridescence", "anisotropy", "transmission", "dispersion",
                            "retroreflection", "diffuseRoughness", "useSSS"}) flags.set(key, model.get(key));
    out.set("flags", flags);
    out.set("distinctModel", !model.sameValue(call(material, "setupLightingModel")));
    const auto clone = call(material, "clone");
    out.set("cloneType", clone.get("type"));
    out.set("cloneColorSame", clone.get("thicknessColorNode").sameValue(color));
    out.set("clonePowerSame", clone.get("thicknessPowerNode").sameValue(power));
    auto destination = ctor.construct();
    auto customScale = object(), copyState = object();
    customScale.set("copy", JSValue::function(
        +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto state = captures[0]; state.set("receiver", self.sameValue(captures[1]));
            state.set("source", args[0].sameValue(captures[2])); return self;
        }, {copyState, customScale, material.get("thicknessScaleNode")}, "copy", 1));
    destination.set("thicknessScaleNode", customScale);
    out.set("copyReturnsSelf", call(destination, "copy", {material}).sameValue(destination));
    out.set("copyColorSame", destination.get("thicknessColorNode").sameValue(color));
    out.set("customScaleRetained", destination.get("thicknessScaleNode").sameValue(customScale));
    out.set("copyState", copyState);
    return out;
}
inline void observed(JSValue target, const char* key, const JSValue& value, const JSValue& events,
                     const std::string& label) {
    JSPropertyDescriptor descriptor;
    descriptor.configurable = descriptor.enumerable = true;
    descriptor.getter = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            push(captures[0], captures[1]); return captures[2];
        }, {events, label, value}, std::string("get ") + key, 0);
    target.defineProperty(key, descriptor);
}
inline JSValue directCase(const JSValue& gate, bool fail = false, bool reenter = false) {
    auto material = nativeMeshSSSNodeMaterialConstructor().construct();
    material.set("thicknessColorNode", node("vec3", {0.8, 0.4, 0.2}));
    const auto model = call(material, "setupLightingModel"), method = model.get("direct");
    auto state = object(), events = JSValue::array(), graphs = JSValue::array(), forwards = JSValue::array();
    state.set("events", events); state.set("graphs", graphs); state.set("forwards", forwards);
    state.set("nested", false); state.set("fail", fail); state.set("reenter", reenter);
    auto input = object(), builder = object(), reflected = object(), diffuse = object();
    const auto direction = node("vec3", {0.0, 0.0, -1.0}), color = node("vec3", {2.0, 3.0, 4.0});
    reflected.set("directDiffuse", diffuse);
    observed(input, "lightDirection", direction, events, "input.lightDirection");
    observed(input, "lightColor", color, events, "input.lightColor");
    observed(input, "reflectedLight", reflected, events, "input.reflectedLight");
    input.set("extra", "must-not-forward");
    observed(model, "useSSS", gate, events, "self.useSSS");
    observed(builder, "material", material, events, "builder.material");
    for (const auto* key : {"thicknessColorNode", "thicknessDistortionNode", "thicknessAmbientNode",
                            "thicknessAttenuationNode", "thicknessPowerNode", "thicknessScaleNode"})
        observed(material, key, material.get(key), events, std::string("material.") + key);
    state.set("model", model); state.set("method", method); state.set("input", input); state.set("builder", builder);
    diffuse.set("addAssign", JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            auto s = captures[0]; push(s.get("events"), "addAssign"); push(s.get("graphs"), graph(args[0]));
            if (s.get("fail").toBoolean()) throw JSTypeError("diffuse-failure");
            if (s.get("reenter").toBoolean() && !s.get("nested").toBoolean()) {
                s.set("nested", true); s.get("method").call(s.get("model"), {s.get("input"), s.get("builder")});
            }
            return "ignored";
        }, {state}, "addAssign", 1));
    auto prototype = model.get("constructor").get("prototype");
    const auto originalBase = prototype.getPrototype();
    auto fixtureBase = JSValue::object(originalBase);
    fixtureBase.set("direct", JSValue::function(
        +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            const auto s = captures[0]; push(s.get("events"), "super.direct");
            auto row = object(); row.set("keys", names(args[0].keys()));
            row.set("receiver", self.sameValue(s.get("model"))); row.set("builder", args[1].sameValue(s.get("builder")));
            row.set("distinctInput", !args[0].sameValue(s.get("input")));
            row.set("direction", args[0].get("lightDirection").sameValue(captures[1]));
            row.set("color", args[0].get("lightColor").sameValue(captures[2]));
            row.set("reflected", args[0].get("reflectedLight").sameValue(captures[3]));
            push(s.get("forwards"), row); return "ignored";
        }, {state, direction, color, reflected}, "direct", 2));
    struct Restore { JSValue prototype, base; ~Restore() { prototype.setPrototype(base); } } restore{prototype, originalBase};
    prototype.setPrototype(fixtureBase);
    auto out = object();
    try { out.set("result", method.call(model, {input, builder}).isUndefined() ? "undefined" : "value"); }
    catch (const JSTypeError&) { out.set("result", "TypeError"); }
    out.set("events", events); out.set("graphs", graphs); out.set("forwards", forwards);
    return out;
}
inline JSValue all() {
    auto out = object(), direct = JSValue::array();
    out.set("metadata", metadata()); out.set("constructorAndCopy", constructorAndCopy());
    for (const auto& gate : Args{false, 1, true}) push(direct, directCase(gate));
    push(direct, directCase(true, true)); push(direct, directCase(true, false, true));
    out.set("direct", direct); return out;
}
} // namespace MeshSSSNodeMaterialScenarios

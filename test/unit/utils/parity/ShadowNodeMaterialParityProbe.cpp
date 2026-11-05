#include "materials/nodes/ShadowNodeMaterial.h"
#include "nodes/functions/ShadowMaskModel.h"

#include "core/JSValue.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue counting(const JSValue& holder) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            target.set("count", JSValue(target.get("count").toNumber() + 1));
            return {};
        },
        {holder}, "assign", 1);
}

// The same structural snapshot the material differential uses: own property names (not Object.keys),
// the kind of each value, and every scalar except the per process id value.
JSValue snapshot(const JSValue& material) {
    std::vector<std::string> sorted;
    for (const auto& name : material.ownPropertyNames()) sorted.push_back(name);
    std::sort(sorted.begin(), sorted.end());

    auto names = JSValue::array();
    auto kinds = JSValue::object(nullptr);
    auto scalars = JSValue::object(nullptr);
    for (const auto& name : sorted) {
        if (name == "uuid") continue;
        names.set(names.length(), JSValue(name));
        const auto value = material.get(name.c_str());
        const char* kind = "object";
        if (value.isNull()) kind = "null";
        else if (value.type() == JSValue::Type::Number) kind = "number";
        else if (value.type() == JSValue::Type::Boolean) kind = "boolean";
        else if (value.type() == JSValue::Type::String) kind = "string";
        else if (value.isCallable()) kind = "function";
        else if (value.isUndefined()) kind = "undefined";
        kinds.set(name.c_str(), JSValue(kind));
        if (name == "id") continue;
        if (value.type() == JSValue::Type::Number || value.type() == JSValue::Type::Boolean ||
            value.type() == JSValue::Type::String) {
            scalars.set(name.c_str(), value);
        }
    }
    auto out = JSValue::object(nullptr);
    out.set("names", names);
    out.set("kinds", kinds);
    out.set("scalars", scalars);
    return out;
}
}  // namespace

int main() {
    try {
        const auto maskCtor = nativeShadowMaskModelConstructor();
        const auto materialCtor = nativeShadowNodeMaterialConstructor();

        const auto model = maskCtor.construct();

        // direct: no shadow on the light node, then one with a shadow.
        auto withoutShadow = JSValue::object(nullptr);
        withoutShadow.set("shadowNode", nullptr);
        auto directInput = JSValue::object(nullptr);
        directInput.set("lightNode", withoutShadow);
        model.get("direct").call(model, {directInput});
        auto withShadow = JSValue::object(nullptr);
        withShadow.set("shadowNode", model.get("shadowNode"));
        auto shadowInput = JSValue::object(nullptr);
        shadowInput.set("lightNode", withShadow);
        model.get("direct").call(model, {shadowInput});

        // finish: the outgoing light rgb assignment is counted.
        auto outgoing = JSValue::object(nullptr);
        outgoing.set("count", 0);
        auto rgb = JSValue::object(nullptr);
        rgb.set("assign", counting(outgoing));
        outgoing.set("rgb", rgb);
        auto context = JSValue::object(nullptr);
        context.set("outgoingLight", outgoing);
        auto finishInput = JSValue::object(nullptr);
        finishInput.set("context", context);
        model.get("finish").call(model, {finishInput});

        const auto material = materialCtor.construct();
        const auto builtModel =
            material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
        const auto secondModel =
            material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});

        auto output = JSValue::object(nullptr);
        output.set("maskName", maskCtor.get("name"));
        output.set("maskLength", maskCtor.get("length"));
        output.set("shadowNodeType", model.get("shadowNode").get("type"));
        output.set("shadowNodeIsNode", model.get("shadowNode").get("isNode"));
        for (const char* hook : {"start", "finish", "direct", "directRectArea", "indirect",
                                 "ambientOcclusion"}) {
            output.set(std::string("hook_") + hook, model.get(hook).get("length"));
        }
        output.set("finishAssigns", outgoing.get("count"));

        output.set("materialName", materialCtor.get("name"));
        output.set("materialLength", materialCtor.get("length"));
        output.set("materialStaticType", materialCtor.get("type"));
        output.set("materialType", material.get("type"));
        output.set("materialFlag", material.get("isShadowNodeMaterial"));
        output.set("materialIsNodeMaterial", material.get("isNodeMaterial"));
        output.set("materialLights", material.get("lights"));
        output.set("materialTransparent", material.get("transparent"));
        output.set("materialSnapshot", snapshot(material));
        output.set("builtShadowNodeType", builtModel.get("shadowNode").get("type"));
        output.set("builtDirectLength", builtModel.get("direct").get("length"));
        output.set("builtFinishLength", builtModel.get("finish").get("length"));
        output.set("distinctModels", !secondModel.sameValue(builtModel));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}

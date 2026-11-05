#include "materials/nodes/MeshMatcapNodeMaterial.h"
#include "materials/nodes/MeshToonNodeMaterial.h"

#include "core/JSValue.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
// The same structural snapshot the material differential uses.
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
        const auto toonCtor = nativeMeshToonNodeMaterialConstructor();
        const auto matcapCtor = nativeMeshMatcapNodeMaterialConstructor();
        const auto toon = toonCtor.construct();
        const auto matcap = matcapCtor.construct();

        const auto model = toon.get("setupLightingModel").call(toon, {JSValue::object(nullptr)});
        const auto secondModel = toon.get("setupLightingModel").call(toon, {JSValue::object(nullptr)});

        auto builderMaterial = JSValue::object(nullptr);
        builderMaterial.set("matcap", nullptr);
        auto builder = JSValue::object(nullptr);
        builder.set("material", builderMaterial);
        bool matcapVariantsBuilt = true;
        try {
            matcap.get("setupVariants").call(matcap, {builder});
        } catch (...) {
            matcapVariantsBuilt = false;
        }

        auto output = JSValue::object(nullptr);
        output.set("toonName", toonCtor.get("name"));
        output.set("toonLength", toonCtor.get("length"));
        output.set("toonStaticType", toonCtor.get("type"));
        output.set("toonType", toon.get("type"));
        output.set("toonFlag", toon.get("isMeshToonNodeMaterial"));
        output.set("toonIsNodeMaterial", toon.get("isNodeMaterial"));
        output.set("toonLights", toon.get("lights"));
        output.set("toonFog", toon.get("fog"));
        output.set("toonGradientMapIsNull", toon.get("gradientMap").isNull());
        output.set("toonSnapshot", snapshot(toon));
        output.set("modelDirectLength", model.get("direct").get("length"));
        output.set("modelIndirectLength", model.get("indirect").get("length"));
        output.set("modelStartLength", model.get("start").get("length"));
        output.set("modelsDistinct", !secondModel.sameValue(model));

        output.set("matcapName", matcapCtor.get("name"));
        output.set("matcapLength", matcapCtor.get("length"));
        output.set("matcapStaticType", matcapCtor.get("type"));
        output.set("matcapType", matcap.get("type"));
        output.set("matcapFlag", matcap.get("isMeshMatcapNodeMaterial"));
        output.set("matcapIsNodeMaterial", matcap.get("isNodeMaterial"));
        output.set("matcapMatcapIsNull", matcap.get("matcap").isNull());
        output.set("matcapSnapshot", snapshot(matcap));
        output.set("matcapVariantsBuilt", matcapVariantsBuilt);
        output.set("toonSource", toonCtor.get("toString").call(toonCtor));
        output.set("matcapSource", matcapCtor.get("toString").call(matcapCtor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}

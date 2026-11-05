#include "materials/nodes/MeshPhongNodeMaterial.h"

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
        const auto ctor = nativeMeshPhongNodeMaterialConstructor();
        const auto material = ctor.construct();

        const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
        const auto secondModel = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});

        auto builderMaterial = JSValue::object(nullptr);
        auto builder = JSValue::object(nullptr);
        builder.set("material", builderMaterial);
        const auto environment = material.get("setupEnvironment").call(material, {builder});
        bool variantsBuilt = true;
        try {
            material.get("setupVariants").call(material, {builder});
        } catch (...) {
            variantsBuilt = false;
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("flag", material.get("isMeshPhongNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("lights", material.get("lights"));
        output.set("shininessNodeIsNull", material.get("shininessNode").isNull());
        output.set("specularNodeIsNull", material.get("specularNode").isNull());
        output.set("shininess", material.get("shininess"));
        output.set("snapshot", snapshot(material));
        output.set("modelSpecular", model.get("specular"));
        output.set("modelDirectLength", model.get("direct").get("length"));
        output.set("modelIndirectLength", model.get("indirect").get("length"));
        output.set("modelsDistinct", !secondModel.sameValue(model));
        output.set("environmentIsNull", environment.isNull());
        output.set("variantsBuilt", variantsBuilt);
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}

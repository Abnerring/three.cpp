#include "materials/nodes/MeshBasicNodeMaterial.h"

#include "core/JSValue.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/PropertyNode.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
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
        const auto ctor = nativeMeshBasicNodeMaterialConstructor();
        const auto material = ctor.construct();

        const auto normal = material.get("setupNormal").call(material, {});
        const auto outgoing = material.get("setupOutgoingLight").call(material, {});
        const auto model = material.get("setupLightingModel").call(material, {});
        const auto secondModel = material.get("setupLightingModel").call(material, {});

        auto plainMaterial = JSValue::object(nullptr);
        plainMaterial.set("lightMap", nullptr);
        auto plainBuilder = JSValue::object(nullptr);
        plainBuilder.set("material", plainMaterial);
        const auto noLightMap = material.get("setupLightMap").call(material, {plainBuilder});
        const auto environment = material.get("setupEnvironment").call(material, {plainBuilder});

        auto mappedMaterial = JSValue::object(nullptr);
        mappedMaterial.set("lightMap", true);
        auto mappedBuilder = JSValue::object(nullptr);
        mappedBuilder.set("material", mappedMaterial);
        const auto lightMap = material.get("setupLightMap").call(material, {mappedBuilder});

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("flag", material.get("isMeshBasicNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("lights", material.get("lights"));
        output.set("fog", material.get("fog"));
        output.set("snapshot", snapshot(material));
        output.set("normalIsNode", normal.get("isNode"));
        output.set("normalDiffers", !normal.sameValue(nativeNormalBinding("normalViewGeometry")));
        output.set("outgoingIsNode", outgoing.get("isNode"));
        output.set("outgoingIsDiffuseRgb",
                   outgoing.sameValue(nativePropertyNodeBinding("diffuseColor").get("rgb")));
        output.set("noLightMapIsNull", noLightMap.isNull());
        output.set("lightMapIsNode", lightMap.get("isNode"));
        output.set("environmentIsNull", environment.isNull());
        output.set("modelIndirectLength", model.get("indirect").get("length"));
        output.set("modelStartLength", model.get("start").get("length"));
        output.set("modelsDistinct", !secondModel.sameValue(model));
        for (const char* method : {"setupNormal", "setupEnvironment", "setupLightMap",
                                   "setupOutgoingLight", "setupLightingModel"}) {
            output.set(std::string(method) + "Length", material.get(method).get("length"));
        }

        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "unknown exception\n";
        return 1;
    }
}

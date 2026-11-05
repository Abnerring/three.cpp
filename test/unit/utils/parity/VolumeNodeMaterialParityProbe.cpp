#include "materials/nodes/VolumeNodeMaterial.h"

#include "core/JSValue.h"

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
        const auto ctor = nativeVolumeNodeMaterialConstructor();
        const auto material = ctor.construct();
        const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
        const auto secondModel =
            material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});

        auto parameters = JSValue::object(nullptr);
        parameters.set("steps", 50);
        parameters.set("transparent", false);
        const auto configured = ctor.construct({parameters});

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("flag", material.get("isVolumeNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("steps", material.get("steps"));
        output.set("offsetNodeIsNull", material.get("offsetNode").isNull());
        output.set("scatteringNodeIsNull", material.get("scatteringNode").isNull());
        output.set("lights", material.get("lights"));
        output.set("transparent", material.get("transparent"));
        output.set("side", material.get("side"));
        output.set("depthTest", material.get("depthTest"));
        output.set("depthWrite", material.get("depthWrite"));
        output.set("snapshot", snapshot(material));
        output.set("setupLightingModelLength", material.get("setupLightingModel").get("length"));
        output.set("modelScatteringLightLength", model.get("scatteringLight").get("length"));
        output.set("modelDirectLength", model.get("direct").get("length"));
        output.set("modelFinishLength", model.get("finish").get("length"));
        output.set("modelsDistinct", !secondModel.sameValue(model));
        output.set("configuredSteps", configured.get("steps"));
        output.set("configuredTransparent", configured.get("transparent"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

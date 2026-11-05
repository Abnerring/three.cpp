#include "materials/nodes/LineDashedNodeMaterial.h"

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
        const auto ctor = nativeLineDashedNodeMaterialConstructor();
        const auto material = ctor.construct();

        bool variantsBuilt = true;
        std::string failure;
        try {
            auto builder = JSValue::object(nullptr);
            material.get("setupVariants").call(material, {builder});
        } catch (const std::exception& error) {
            variantsBuilt = false;
            failure = error.what();
        } catch (...) {
            variantsBuilt = false;
            failure = "unknown";
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("flag", material.get("isLineDashedNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("dashOffset", material.get("dashOffset"));
        output.set("dashSize", material.get("dashSize"));
        output.set("gapSize", material.get("gapSize"));
        for (const char* field : {"offsetNode", "dashScaleNode", "dashSizeNode", "gapSizeNode"}) {
            output.set(std::string(field) + "IsNull", material.get(field).isNull());
        }
        output.set("snapshot", snapshot(material));
        output.set("setupVariantsLength", material.get("setupVariants").get("length"));
        output.set("variantsBuilt", variantsBuilt);
        if (!variantsBuilt) output.set("variantFailure", JSValue(failure));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

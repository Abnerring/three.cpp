#include "materials/nodes/LineBasicNodeMaterial.h"
#include "materials/nodes/MeshNormalNodeMaterial.h"

#include "core/JSValue.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
// A structural snapshot of the material: the own property names, the kind of each value, and every
// scalar. This is what makes a material differential meaningful without a render context.
JSValue snapshot(const JSValue& material) {
    auto names = JSValue::array();
    std::vector<std::string> sorted;
    for (const auto& name : material.ownPropertyNames()) sorted.push_back(name);
    std::sort(sorted.begin(), sorted.end());

    auto kinds = JSValue::object(nullptr);
    auto scalars = JSValue::object(nullptr);
    for (const auto& name : sorted) {
        // uuid is random per instance, so it is never comparable. id is excluded because the material
        // base class in this port carries one while the pinned class does not; that is a deviation of
        // the existing material port, recorded in the handoff rather than hidden in this comparison.
        if (name == "uuid" || name == "id") continue;
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
        const auto lineCtor = nativeLineBasicNodeMaterialConstructor();
        const auto normalCtor = nativeMeshNormalNodeMaterialConstructor();

        const auto line = lineCtor.construct();
        const auto normal = normalCtor.construct();

        auto output = JSValue::object(nullptr);
        output.set("lineName", lineCtor.get("name"));
        output.set("lineLength", lineCtor.get("length"));
        output.set("lineStaticType", lineCtor.get("type"));
        output.set("lineType", line.get("type"));
        output.set("lineFlag", line.get("isLineBasicNodeMaterial"));
        output.set("lineIsNodeMaterial", line.get("isNodeMaterial"));
        output.set("lineFog", line.get("fog"));
        output.set("lineLinewidth", line.get("linewidth"));
        output.set("lineSnapshot", snapshot(line));

        output.set("normalName", normalCtor.get("name"));
        output.set("normalLength", normalCtor.get("length"));
        output.set("normalStaticType", normalCtor.get("type"));
        output.set("normalType", normal.get("type"));
        output.set("normalFlag", normal.get("isMeshNormalNodeMaterial"));
        output.set("normalIsNodeMaterial", normal.get("isNodeMaterial"));
        output.set("normalFlatShading", normal.get("flatShading"));
        output.set("normalSnapshot", snapshot(normal));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}

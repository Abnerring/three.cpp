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
        // uuid is random per instance, so its name is skipped entirely.
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
        // The material id counter is per process, so the value cannot line up. The name and the kind
        // still have to, and the descriptor below pins how id is defined.
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

// Both the pinned source and this port define the material id with Object.defineProperty and a
// descriptor that only carries a value, which makes it an own property that Object.keys does not list.
// The descriptor is compared so that claim is checked instead of asserted.
JSValue idDescriptor(const JSValue& material) {
    auto out = JSValue::object(nullptr);
    const auto descriptor = material.getOwnPropertyDescriptor("id");
    out.set("present", JSValue(descriptor.has_value()));
    out.set("writable", JSValue(descriptor ? descriptor->writable.value_or(false) : false));
    out.set("enumerable", JSValue(descriptor ? descriptor->enumerable.value_or(false) : false));
    out.set("configurable", JSValue(descriptor ? descriptor->configurable.value_or(false) : false));
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
        output.set("lineIdDescriptor", idDescriptor(line));
        output.set("lineFog", line.get("fog"));
        output.set("lineLinewidth", line.get("linewidth"));
        output.set("lineSnapshot", snapshot(line));

        output.set("normalName", normalCtor.get("name"));
        output.set("normalLength", normalCtor.get("length"));
        output.set("normalStaticType", normalCtor.get("type"));
        output.set("normalType", normal.get("type"));
        output.set("normalFlag", normal.get("isMeshNormalNodeMaterial"));
        output.set("normalIsNodeMaterial", normal.get("isNodeMaterial"));
        output.set("normalIdDescriptor", idDescriptor(normal));
        output.set("normalFlatShading", normal.get("flatShading"));
        output.set("normalSnapshot", snapshot(normal));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
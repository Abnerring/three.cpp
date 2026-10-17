// Differential probe for the native src/loaders/ObjectLoader.js port.
//
// Each stdin line is a serialised scene document. It is parsed and the resulting tree is printed as a
// structural summary: the type, the name and uuid, the transform, the flags, and the children recursively.
// Comparing summaries rather than objects keeps the output small while still covering the parts a hand port
// drifts on - which branch built the object, what the epilogue set, and in which order the children appear.
#include "animation/AnimationUtils.h"
#include "core/JsonValue.h"
#include "core/Object3D.h"
#include "loaders/ObjectLoader.h"
#include <cmath>
#include <iostream>
#include <string>

namespace {
JsonValue encode(double value) {
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
JsonValue triple(double x, double y, double z) {
    return JsonValue(JsonValue::Object{{"x", encode(x)}, {"y", encode(y)}, {"z", encode(z)}});
}
JsonValue summary(const std::shared_ptr<Object3D>& object, int depth) {
    if (!object || depth > 6) return JsonValue(JsonValue::Object{{"cut", true}});
    JsonValue::Array children;
    for (const auto& child : object->children) children.emplace_back(summary(child, depth + 1));
    return JsonValue(JsonValue::Object{
        {"type", object->type},
        {"name", object->name},
        {"uuid", object->uuid},
        {"position", triple(object->position.x, object->position.y, object->position.z)},
        {"quaternion", triple(object->quaternion.x, object->quaternion.y, object->quaternion.z)},
        {"scale", triple(object->scale.x, object->scale.y, object->scale.z)},
        {"visible", object->visible},
        {"renderOrder", encode(object->renderOrder)},
        {"layers", encode(object->layers.mask.value().toNumber())},
        {"children", JsonValue(children)}});
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            ObjectLoader loader;
            const auto result = loader.parse(Animation_detail::parseJSON(line));
            std::cout << summary(result.object, 0).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
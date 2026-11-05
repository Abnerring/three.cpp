#include "environments/DebugEnvironment.h"

#include "core/EventDispatcher.h"
#include "core/EventDispatcher.h"
#include "core/JSValue.h"
#include "lights/PointLight.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "objects/Mesh.h"

#include <iostream>
#include <memory>

namespace {
JSValue vec3Value(const Vector3& vector) {
    auto value = JSValue::array();
    // The index must be a JSValue: a literal 0 selects the null const char* overload.
    value.set(JSValue(0.0), JSValue(vector.x));
    value.set(JSValue(1.0), JSValue(vector.y));
    value.set(JSValue(2.0), JSValue(vector.z));
    return value;
}
}  // namespace

int main() {
    try {
        DebugEnvironment environment;

        const auto room = std::dynamic_pointer_cast<Mesh>(environment.children[0]);
        const auto roomMaterial = room ? std::dynamic_pointer_cast<MeshStandardMaterial>(room->material) : nullptr;
        const auto mainLight =
            std::dynamic_pointer_cast<PointLight>(environment.children[1]);
        const auto first = std::dynamic_pointer_cast<Mesh>(environment.children[2]);
        const auto second = std::dynamic_pointer_cast<Mesh>(environment.children[3]);
        const auto third = std::dynamic_pointer_cast<Mesh>(environment.children[4]);
        const auto firstMaterial = first ? std::dynamic_pointer_cast<MeshLambertMaterial>(first->material) : nullptr;
        const auto secondMaterial = second ? std::dynamic_pointer_cast<MeshLambertMaterial>(second->material) : nullptr;
        const auto thirdMaterial = third ? std::dynamic_pointer_cast<MeshLambertMaterial>(third->material) : nullptr;

        int geometryEvents = 0;
        int materialEvents = 0;
        if (room && room->geometry) {
            room->geometry->addEventListener("dispose", [&geometryEvents](Event&) { ++geometryEvents; });
        }
        for (const auto& child : environment.children) {
            const auto mesh = std::dynamic_pointer_cast<Mesh>(child);
            if (mesh && mesh->material) {
                mesh->material->addEventListener("dispose", [&materialEvents](Event&) { ++materialEvents; });
            }
        }
        environment.dispose();
        environment.dispose();
        const auto disposedGeometryEvents = geometryEvents;
        const auto disposedMaterialEvents = materialEvents;

        auto output = JSValue::object(nullptr);
        output.set("childrenCount", JSValue(static_cast<double>(environment.children.size())));
        output.set("roomIsMesh", JSValue(room != nullptr));
        output.set("roomHasUv", JSValue(room && room->geometry ? room->geometry->getAttribute("uv") != nullptr : false));
        output.set("roomMetalness", JSValue(roomMaterial ? roomMaterial->metalness : -1.0));
        output.set("roomSide", JSValue(roomMaterial ? roomMaterial->side : -999));
        output.set("roomScale", room ? vec3Value(room->scale) : JSValue::array());
        output.set("mainLightIsPointLight", JSValue(mainLight != nullptr));
        output.set("mainLightIntensity", JSValue(mainLight ? mainLight->intensity : -1.0));
        output.set("mainLightDistance", JSValue(mainLight ? mainLight->distance : -1.0));
        output.set("mainLightDecay", JSValue(mainLight ? mainLight->decay : -1.0));
        output.set("firstPosition", first ? vec3Value(first->position) : JSValue::array());
        output.set("firstScale", first ? vec3Value(first->scale) : JSValue::array());
        output.set("firstColor", JSValue(firstMaterial ? firstMaterial->color.getHex() : -1));
        output.set("firstEmissive", JSValue(firstMaterial ? firstMaterial->emissive.getHex() : -1));
        output.set("firstEmissiveIntensity", JSValue(firstMaterial ? firstMaterial->emissiveIntensity : -1.0));
        output.set("secondPosition", second ? vec3Value(second->position) : JSValue::array());
        output.set("secondScale", second ? vec3Value(second->scale) : JSValue::array());
        output.set("secondColor", JSValue(secondMaterial ? secondMaterial->color.getHex() : -1));
        output.set("thirdPosition", third ? vec3Value(third->position) : JSValue::array());
        output.set("thirdScale", third ? vec3Value(third->scale) : JSValue::array());
        output.set("thirdColor", JSValue(thirdMaterial ? thirdMaterial->color.getHex() : -1));
        output.set("sharedGeometry", JSValue(room && first && room->geometry == first->geometry));
        output.set("geometryDisposeEvents", JSValue(static_cast<double>(disposedGeometryEvents)));
        output.set("materialDisposeEvents", JSValue(static_cast<double>(disposedMaterialEvents)));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

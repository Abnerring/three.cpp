#include "environments/ColorEnvironment.h"

#include "core/EventDispatcher.h"
#include "core/JSValue.h"
#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"

#include <iostream>
#include <memory>

int main() {
    try {
        ColorEnvironment environment;
        const auto mesh = std::dynamic_pointer_cast<Mesh>(environment.children[0]);
        const auto geometry = mesh ? std::dynamic_pointer_cast<SphereGeometry>(mesh->geometry) : nullptr;
        const auto material = mesh ? std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material) : nullptr;

        int geometryEvents = 0;
        int materialEvents = 0;
        if (mesh && mesh->geometry) {
            mesh->geometry->addEventListener("dispose", [&geometryEvents](Event&) { ++geometryEvents; });
        }
        if (mesh && mesh->material) {
            mesh->material->addEventListener("dispose", [&materialEvents](Event&) { ++materialEvents; });
        }
        environment.dispose();
        environment.dispose();

        ColorEnvironment coloured(0x336699);
        const auto colouredMesh = std::dynamic_pointer_cast<Mesh>(coloured.children[0]);
        const auto colouredMaterial =
            colouredMesh ? std::dynamic_pointer_cast<MeshBasicMaterial>(colouredMesh->material) : nullptr;

        auto output = JSValue::object(nullptr);
        output.set("name", JSValue(environment.name));
        output.set("childrenCount", JSValue(static_cast<double>(environment.children.size())));
        output.set("childType", JSValue(mesh ? mesh->type : std::string{}));
        output.set("geometryType", JSValue(geometry ? geometry->type : std::string{}));
        output.set("radius", JSValue(geometry ? geometry->parameters.radius : -1.0));
        output.set("widthSegments", JSValue(geometry ? geometry->parameters.widthSegments : -1.0));
        output.set("heightSegments", JSValue(geometry ? geometry->parameters.heightSegments : -1.0));
        output.set("materialType", JSValue(material ? material->type : std::string{}));
        output.set("materialSide", JSValue(material ? material->side : -999));
        output.set("defaultColorHex", JSValue(material ? material->color.getHex() : -1));
        output.set("customColorHex", JSValue(colouredMaterial ? colouredMaterial->color.getHex() : -1));
        output.set("geometryDisposeEvents", JSValue(geometryEvents));
        output.set("materialDisposeEvents", JSValue(materialEvents));
        output.set("isScene", JSValue(environment.isScene));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

#include "generators/city/InstancedMeshGenerator.h"

#include "core/EventDispatcher.h"
#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Matrix4.h"

#include <iostream>
#include <memory>
#include <vector>

int main() {
    try {
        JsonValue parameters = JsonValue::Object{};
        parameters["size"] = JsonValue(1);
        JsonValue defaults = JsonValue::Object{};
        defaults["fromDefaults"] = JsonValue(7);
        defaults["size"] = JsonValue(99);
        JsonValue factoryParameters;
        auto createGeometry = [&factoryParameters](const JsonValue& received) {
            factoryParameters = received;
            return std::make_shared<BoxGeometry>();
        };
        auto createMaterial = [] { return std::make_shared<MeshBasicMaterial>(); };
        InstancedMeshGenerator generator(parameters, createGeometry, createMaterial, "city", false,
                                          defaults);

        std::vector<Matrix4> placements;
        for (int i = 0; i < 2; ++i) {
            Matrix4 matrix;
            matrix.makeTranslation(static_cast<double>(i + 1), 2.0, 3.0);
            placements.push_back(matrix);
        }
        const auto mesh = generator.build(placements);
        int meshDisposeEvents = 0;
        if (mesh) {
            mesh->addEventListener("dispose", [&meshDisposeEvents](Event&) { ++meshDisposeEvents; });
        }

        auto translations = JSValue::array();
        std::size_t index = 0;
        for (double i = 0; i < static_cast<double>(placements.size()); ++i) {
            Matrix4 matrix;
            mesh->getMatrixAt(i, matrix);
            translations.set(index++, JSValue(matrix.elements()[12]));
            translations.set(index++, JSValue(matrix.elements()[13]));
            translations.set(index++, JSValue(matrix.elements()[14]));
        }

        // Count the dispose events of the first resources so a rebuild can be checked.
        int firstGeometryDisposeEvents = 0;
        int firstMaterialDisposeEvents = 0;
        if (generator.geometry) {
            generator.geometry->addEventListener(
                "dispose", [&firstGeometryDisposeEvents](Event&) { ++firstGeometryDisposeEvents; });
        }
        if (generator.material) {
            generator.material->addEventListener(
                "dispose", [&firstMaterialDisposeEvents](Event&) { ++firstMaterialDisposeEvents; });
        }

        // Capacity growth: a third placement no longer fits the capacity of two.
        std::vector<Matrix4> threePlacements = placements;
        Matrix4 extra;
        extra.makeTranslation(3.0, 2.0, 3.0);
        threePlacements.push_back(extra);
        const auto grownMesh = generator.build(threePlacements);

        // Parameter change: the same placements rebuild because the parameters key changed.
        generator.parameters["size"] = JsonValue(2);
        const auto rebuiltMesh = generator.build(placements);

        // The factory received the Object.assign( {}, defaults, parameters ) view.

        const double factoryParameterKeys = static_cast<double>(factoryParameters.size());
        const bool factoryHasDefaultKey = factoryParameters.contains("fromDefaults");
        const double factorySizeValue = factoryParameters.at("size").isNull()
                                            ? -1
                                            : factoryParameters.at("size").get<double>();

        auto output = JSValue::object(nullptr);
        output.set("factoryParameterKeys", JSValue(factoryParameterKeys));
        output.set("factoryHasDefaultKey", JSValue(factoryHasDefaultKey));
        output.set("factorySizeValue", JSValue(factorySizeValue));
        output.set("firstGeometryDisposeEvents", JSValue(firstGeometryDisposeEvents));
        output.set("firstMaterialDisposeEvents", JSValue(firstMaterialDisposeEvents));
        output.set("meshDisposeEvents", JSValue(meshDisposeEvents));
        output.set("grownCapacity", JSValue(grownMesh->instanceMatrix->getCount()));
        output.set("grownCount", JSValue(static_cast<double>(grownMesh->count)));
        output.set("rebuiltCapacity", JSValue(rebuiltMesh->instanceMatrix->getCount()));
        output.set("rebuiltCount", JSValue(static_cast<double>(rebuiltMesh->count)));
        output.set("rebuiltTranslations", [&] {
            auto values = JSValue::array();
            std::size_t i = 0;
            for (double index = 0; index < static_cast<double>(placements.size()); ++index) {
                Matrix4 matrix;
                rebuiltMesh->getMatrixAt(index, matrix);
                values.set(i++, JSValue(matrix.elements()[12]));
                values.set(i++, JSValue(matrix.elements()[13]));
                values.set(i++, JSValue(matrix.elements()[14]));
            }
            return values;
        }());
        output.set("capacity", JSValue(mesh->instanceMatrix->getCount()));
        output.set("count", JSValue(static_cast<double>(mesh->count)));
        output.set("visible", JSValue(mesh->visible));
        output.set("name", JSValue(mesh->name));
        output.set("castShadow", JSValue(mesh->castShadow));
        output.set("receiveShadow", JSValue(mesh->receiveShadow));
        output.set("translations", translations);
        output.set("geometryIsBox", JSValue(dynamic_cast<BoxGeometry*>(generator.geometry.get()) != nullptr));
        output.set("materialIsBasic",
                   JSValue(dynamic_cast<MeshBasicMaterial*>(generator.material.get()) != nullptr));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
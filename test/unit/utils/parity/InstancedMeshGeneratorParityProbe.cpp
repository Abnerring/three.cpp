#include "generators/city/InstancedMeshGenerator.h"

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
        auto createGeometry = [](const JsonValue&) { return std::make_shared<BoxGeometry>(); };
        auto createMaterial = [] { return std::make_shared<MeshBasicMaterial>(); };
        InstancedMeshGenerator generator(parameters, createGeometry, createMaterial, "city", false);

        std::vector<Matrix4> placements;
        for (int i = 0; i < 2; ++i) {
            Matrix4 matrix;
            matrix.makeTranslation(static_cast<double>(i + 1), 2.0, 3.0);
            placements.push_back(matrix);
        }
        const auto mesh = generator.build(placements);

        auto translations = JSValue::array();
        std::size_t index = 0;
        for (double i = 0; i < static_cast<double>(placements.size()); ++i) {
            Matrix4 matrix;
            mesh->getMatrixAt(i, matrix);
            translations.set(index++, JSValue(matrix.elements()[12]));
            translations.set(index++, JSValue(matrix.elements()[13]));
            translations.set(index++, JSValue(matrix.elements()[14]));
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

        auto output = JSValue::object(nullptr);
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
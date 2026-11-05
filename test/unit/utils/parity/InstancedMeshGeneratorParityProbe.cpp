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

        auto output = JSValue::object(nullptr);
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
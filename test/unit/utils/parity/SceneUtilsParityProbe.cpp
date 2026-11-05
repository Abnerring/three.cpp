#include "utils/SceneUtils.h"

#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Group.h"
#include "objects/InstancedMesh.h"
#include "objects/Mesh.h"

#include <iostream>
#include <memory>
#include <vector>

int main() {
    try {
        auto geometry = std::make_shared<BoxGeometry>();
        auto material = std::make_shared<MeshBasicMaterial>();
        auto instanced = std::make_shared<InstancedMesh>(geometry, material, JSValue(3));
        Matrix4 matrix;
        matrix.makeTranslation(1, 2, 3);
        instanced->setMatrixAt(0, matrix);
        matrix.makeTranslation(4, 5, 6);
        instanced->setMatrixAt(1, matrix);
        matrix.makeTranslation(7, 8, 9);
        instanced->setMatrixAt(2, matrix);
        instanced->name = "instanced";
        const auto group = SceneUtils::createMeshesFromInstancedMesh(instanced);

        auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                           std::make_shared<MeshBasicMaterial>());
        auto scene = std::make_shared<Object3D>();
        scene->add(mesh);
        const double vertexCount = SceneUtils::reduceVertices(
            scene, [](double value, const Vector3&) { return value + 1; },
            0);
        const double sumY = SceneUtils::reduceVertices(
            scene,
            [](double value, const Vector3& vertex) { return value + vertex.y; }, 0);
        const double minX = SceneUtils::reduceVertices(
            scene,
            [](double value, const Vector3& vertex) { return value < vertex.x ? value : vertex.x; },
            1e9);

        std::vector<std::shared_ptr<Material>> materials{std::make_shared<MeshBasicMaterial>(),
                                                        std::make_shared<MeshBasicMaterial>()};
        const auto multi = SceneUtils::createMultiMaterialObject(geometry, materials);

        auto output = JSValue::object(nullptr);
        output.set("groupChildCount", JSValue(static_cast<double>(group->children.size())));
        output.set("groupName", JSValue(group->name));
        output.set("firstPosition", [&] {
            auto position = JSValue::array();
            const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
            position.set(JSValue(0.0), JSValue(first ? first->position.x : -1));
            position.set(JSValue(1.0), JSValue(first ? first->position.y : -1));
            position.set(JSValue(2.0), JSValue(first ? first->position.z : -1));
            return position;
        }());
        output.set("multiMaterialCount", JSValue(static_cast<double>(multi->children.size())));
        output.set("vertexCount", JSValue(vertexCount));
        output.set("sumY", JSValue(sumY));
        output.set("minX", JSValue(minX));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

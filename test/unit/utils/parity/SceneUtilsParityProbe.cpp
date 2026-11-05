#include "utils/SceneUtils.h"

#include "core/InstancedBufferAttribute.h"
#include "core/JSValue.h"
#include "math/Color.h"
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

        // Sort a second instance mesh whose colours and instanced attribute must travel with it.
        auto payloads = std::make_shared<InstancedMesh>(geometry, material, JSValue(3));
        Matrix4 payloadMatrix;
        payloadMatrix.makeTranslation(3, 0, 0);
        payloads->setMatrixAt(0, payloadMatrix);
        payloadMatrix.makeTranslation(1, 0, 0);
        payloads->setMatrixAt(1, payloadMatrix);
        payloadMatrix.makeTranslation(2, 0, 0);
        payloads->setMatrixAt(2, payloadMatrix);
        payloads->setColorAt(0, Color(1, 0, 0));
        payloads->setColorAt(1, Color(0, 1, 0));
        payloads->setColorAt(2, Color(0, 0, 1));
        auto instanceValues =
            std::make_shared<InstancedBufferAttribute>(std::vector<float>{30, 10, 20}, 1);
        payloads->geometry->setAttribute("instanceValue", instanceValues);
        SceneUtils::sortInstancedMesh(payloads, [&](double a, double b) {
            Matrix4 left;
            Matrix4 right;
            payloads->getMatrixAt(a, left);
            payloads->getMatrixAt(b, right);
            return left.elements()[12] - right.elements()[12];
        });
        auto payloadColours = JSValue::array();
        auto payloadValues = JSValue::array();
        auto payloadPositions = JSValue::array();
        for (double i = 0; i < payloads->count; ++i) {
            Color colour;
            payloads->getColorAt(i, colour);
            payloadColours.set(JSValue(i * 3), JSValue(colour.r));
            payloadColours.set(JSValue(i * 3 + 1), JSValue(colour.g));
            payloadColours.set(JSValue(i * 3 + 2), JSValue(colour.b));
            payloadValues.set(JSValue(i), JSValue(instanceValues->getX(i)));
            Matrix4 current;
            payloads->getMatrixAt(i, current);
            payloadPositions.set(JSValue(i), JSValue(current.elements()[12]));
        }

        auto output = JSValue::object(nullptr);
        output.set("payloadColours", payloadColours);
        output.set("payloadValues", payloadValues);
        output.set("payloadPositions", payloadPositions);
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

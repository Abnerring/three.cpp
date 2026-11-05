#include "utils/SceneUtils.h"

#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/InstancedMesh.h"

#include <iostream>
#include <memory>

namespace {
JSValue instancePositions(const std::shared_ptr<InstancedMesh>& mesh) {
    auto positions = JSValue::array();
    std::size_t index = 0;
    for (double i = 0; i < mesh->count; ++i) {
        Matrix4 matrix;
        mesh->getMatrixAt(i, matrix);
        positions.set(index++, JSValue(matrix.elements()[12]));
        positions.set(index++, JSValue(matrix.elements()[13]));
        positions.set(index++, JSValue(matrix.elements()[14]));
    }
    return positions;
}
}  // namespace

int main() {
    try {
        auto geometry = std::make_shared<BoxGeometry>();
        auto material = std::make_shared<MeshBasicMaterial>();
        auto instanced = std::make_shared<InstancedMesh>(geometry, material, JSValue(3));
        Matrix4 matrix;
        matrix.makeTranslation(3, 0, 0);
        instanced->setMatrixAt(0, matrix);
        matrix.makeTranslation(1, 2, 0);
        instanced->setMatrixAt(1, matrix);
        matrix.makeTranslation(2, 1, 0);
        instanced->setMatrixAt(2, matrix);

        const auto before = instancePositions(instanced);
        // Sort ascending by the instance translation x, read from the matrix payload.
        SceneUtils::sortInstancedMesh(instanced, [&](double a, double b) {
            Matrix4 left;
            Matrix4 right;
            instanced->getMatrixAt(a, left);
            instanced->getMatrixAt(b, right);
            return left.elements()[12] - right.elements()[12];
        });
        const auto after = instancePositions(instanced);

        auto output = JSValue::object(nullptr);
        output.set("before", before);
        output.set("after", after);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

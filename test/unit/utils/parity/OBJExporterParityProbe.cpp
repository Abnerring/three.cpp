#include "exporters/OBJExporter.h"

#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"

#include <iostream>
#include <memory>

int main() {
    try {
        auto geometry = std::make_shared<BoxGeometry>();
        auto material = std::make_shared<MeshBasicMaterial>();
        material->name = "mat";
        auto mesh = std::make_shared<Mesh>(geometry, material);
        mesh->name = "box";
        mesh->matrixWorld.makeTranslation(1, 2, 3);
        Object3D scene;
        scene.add(mesh);

        const std::string obj = OBJExporter().parse(scene);
        auto output = JSValue::object(nullptr);
        output.set("obj", JSValue(obj));
        output.set("lineCount", JSValue(static_cast<double>(std::count(obj.begin(), obj.end(), '\n'))));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

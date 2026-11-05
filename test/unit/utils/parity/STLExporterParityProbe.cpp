#include "exporters/STLExporter.h"

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
        auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
        mesh->matrixWorld.makeTranslation(1, 2, 3);
        Object3D scene;
        scene.add(mesh);

        STLExporter exporter;
        const std::string ascii = exporter.parse(scene);
        const std::string binary = exporter.parse(scene, true);
        auto binaryBytes = JSValue::array();
        for (std::size_t i = 0; i < binary.size(); ++i) {
            binaryBytes.set(i, JSValue(static_cast<double>(static_cast<unsigned char>(binary[i]))));
        }

        auto output = JSValue::object(nullptr);
        output.set("ascii", JSValue(ascii));
        output.set("lineCount", JSValue(static_cast<double>(std::count(ascii.begin(), ascii.end(), '\n'))));
        output.set("binaryBytes", binaryBytes);
        output.set("binarySize", JSValue(static_cast<double>(binary.size())));
        output.set("startsWithSolid", JSValue(ascii.rfind("solid exported\n", 0) == 0));
        output.set("endsWithEndsolid", JSValue(ascii.size() >= 18 &&
                                               ascii.rfind("endsolid exported\n") == ascii.size() - 18));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

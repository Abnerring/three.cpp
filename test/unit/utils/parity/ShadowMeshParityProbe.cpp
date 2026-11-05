#include "objects/ShadowMesh.h"

#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "math/Plane.h"
#include "math/Vector4.h"
#include "materials/MeshBasicMaterial.h"

#include <iostream>
#include <memory>

namespace {
JSValue elementsValue(const Matrix4& matrix) {
    auto values = JSValue::array();
    const auto elements = matrix.elements();
    std::size_t index = 0;
    for (const double element : elements) values.set(index++, JSValue(element));
    return values;
}
}  // namespace

int main() {
    try {
        auto source = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                             std::make_shared<MeshBasicMaterial>());
        ShadowMesh shadow(source);
        const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(shadow.material);

        const Plane plane(Vector3(0, 1, 0), -2);
        const Vector4 light(4, 5, 6, 1);
        shadow.update(plane, light);
        const auto identityMatrix = elementsValue(shadow.matrix);

        source->matrixWorld.makeTranslation(1, 2, 3);
        shadow.update(plane, light);
        const auto translatedMatrix = elementsValue(shadow.matrix);

        auto output = JSValue::object(nullptr);
        output.set("sameGeometry", JSValue(shadow.geometry == source->geometry));
        output.set("materialIsBasic", JSValue(material != nullptr));
        output.set("materialColor", JSValue(material ? material->color.getHex() : -1));
        output.set("transparent", JSValue(material ? material->transparent : false));
        output.set("opacity", JSValue(material ? material->opacity : -1.0));
        output.set("depthWrite", JSValue(material ? material->depthWrite : true));
        output.set("stencilWrite", JSValue(material ? material->stencilWrite : false));
        output.set("stencilFunc", JSValue(material ? material->stencilFunc : -1));
        output.set("stencilRef", JSValue(material ? material->stencilRef : -1));
        output.set("stencilZPass", JSValue(material ? material->stencilZPass : -1));
        output.set("isShadowMesh", JSValue(shadow.isShadowMesh));
        output.set("frustumCulled", JSValue(shadow.frustumCulled));
        output.set("matrixAutoUpdate", JSValue(shadow.matrixAutoUpdate));
        output.set("referencesSourceMatrix", JSValue(shadow.meshMatrix == &source->matrixWorld));
        output.set("identityMatrix", identityMatrix);
        output.set("translatedMatrix", translatedMatrix);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

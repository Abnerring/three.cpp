#include "helpers/RectAreaLightHelper.h"

#include "core/JSValue.h"
#include "lights/RectAreaLight.h"
#include "materials/LineBasicMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"

#include <algorithm>
#include <iostream>
#include <memory>

namespace {
JSValue attributeValues(const std::shared_ptr<BufferGeometry>& geometry) {
    auto values = JSValue::array();
    const auto position = geometry->getAttribute("position");
    std::size_t index = 0;
    for (double i = 0; i < position->getCount(); ++i) {
        values.set(index++, JSValue(position->getX(i)));
        values.set(index++, JSValue(position->getY(i)));
        values.set(index++, JSValue(position->getZ(i)));
    }
    return values;
}

JSValue matrixValues(const Matrix4& matrix) {
    auto values = JSValue::array();
    std::size_t index = 0;
    for (const double element : matrix.elements()) values.set(index++, JSValue(element));
    return values;
}
}  // namespace

int main() {
    try {
        auto light = std::make_shared<RectAreaLight>(Color(0xff0000), 2, 4, 6);
        light->matrixWorld.makeTranslation(1, 2, 3);
        RectAreaLightHelper helper(light);
        helper.updateMatrixWorld();

        auto explicitLight = std::make_shared<RectAreaLight>(Color(0xff0000), 2, 4, 6);
        RectAreaLightHelper explicitHelper(explicitLight, Color(0x00ff00));
        explicitHelper.updateMatrixWorld();

        const auto fill = std::dynamic_pointer_cast<Mesh>(helper.children[0]);
        const auto fillMaterial = fill ? std::dynamic_pointer_cast<MeshBasicMaterial>(fill->material) : nullptr;
        const auto outlineMaterial = std::dynamic_pointer_cast<LineBasicMaterial>(helper.material);
        const auto explicitOutline =
            std::dynamic_pointer_cast<LineBasicMaterial>(explicitHelper.material);
        const auto explicitFill = explicitHelper.children.empty()
            ? nullptr
            : std::dynamic_pointer_cast<Mesh>(explicitHelper.children[0]);

        auto output = JSValue::object(nullptr);
        output.set("type", JSValue(helper.type));
        output.set("outlinePositions", attributeValues(helper.geometry));
        output.set("fillPositions", fill ? attributeValues(fill->geometry) : JSValue::array());
        output.set("outlineFog", JSValue(outlineMaterial ? outlineMaterial->fog : true));
        output.set("fillSide", JSValue(fillMaterial ? fillMaterial->side : -999));
        output.set("fillFog", JSValue(fillMaterial ? fillMaterial->fog : true));
        output.set("scale", JSValue::array());
        {
            auto scale = JSValue::array();
            scale.set(JSValue(0.0), JSValue(helper.scale.x));
            scale.set(JSValue(1.0), JSValue(helper.scale.y));
            scale.set(JSValue(2.0), JSValue(helper.scale.z));
            output.set("scale", scale);
        }
        output.set("outlineColorHex", JSValue(outlineMaterial ? outlineMaterial->color.getHex() : -1));
        output.set("fillColorHex", JSValue(fillMaterial ? fillMaterial->color.getHex() : -1));
        output.set("explicitOutlineColorHex",
                   JSValue(explicitOutline ? explicitOutline->color.getHex() : -1));
        output.set("explicitFillColorHex",
                   JSValue(explicitFill
                               ? std::dynamic_pointer_cast<MeshBasicMaterial>(explicitFill->material)
                                     ->color.getHex()
                               : -1));
        output.set("matrixWorld", matrixValues(helper.matrixWorld));
        bool fillMatrixEqualsHelper = false;
        if (fill) {
            const auto helperElements = helper.matrixWorld.elements();
            const auto fillElements = fill->matrixWorld.elements();
            fillMatrixEqualsHelper =
                std::equal(helperElements.begin(), helperElements.end(), fillElements.begin());
        }
        output.set("fillMatrixEqualsHelper", JSValue(fillMatrixEqualsHelper));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

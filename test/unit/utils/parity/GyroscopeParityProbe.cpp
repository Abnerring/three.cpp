#include "misc/Gyroscope.h"

#include "core/JSValue.h"
#include "math/Euler.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#include <iostream>
#include <memory>

namespace {
void writeElements(JSValue& target, const char* key, const Matrix4& matrix) {
    auto values = JSValue::array();
    const auto elements = matrix.elements();
    for (std::size_t i = 0; i < 16; ++i) values.set(i, JSValue(static_cast<double>(elements[i])));
    target.set(key, values);
}
}  // namespace

int main() {
    try {
        auto output = JSValue::object(nullptr);

        // Scenario one: no parent, so the world matrix is the local matrix.
        Gyroscope standalone;
        standalone.position.set(1, 2, 3);
        standalone.quaternion.setFromEuler(Euler(0.3, 0.5, 0.7));
        standalone.scale.set(2, 3, 4);
        standalone.updateMatrixWorld();
        writeElements(output, "standaloneMatrix", standalone.matrix);
        writeElements(output, "standaloneWorld", standalone.matrixWorld);

        // Scenario two: under a rotated, translated parent the world translation is the parent's while
        // the rotation stays local.
        auto parent = std::make_shared<Object3D>();
        parent->position.set(10, 20, 30);
        parent->quaternion.setFromEuler(Euler(0.1, 0.2, 0.3));
        auto gyroscope = std::make_shared<Gyroscope>();
        gyroscope->position.set(1, 2, 3);
        gyroscope->quaternion.setFromEuler(Euler(0.4, 0.5, 0.6));
        gyroscope->scale.set(1.5, 1.5, 1.5);
        parent->add(gyroscope);
        parent->updateMatrixWorld(true);
        writeElements(output, "parentWorld", parent->matrixWorld);
        writeElements(output, "attachedWorld", gyroscope->matrixWorld);

        // Scenario three: the forced update reaches the children.
        auto host = std::make_shared<Gyroscope>();
        host->position.set(0, 5, 0);
        auto child = std::make_shared<Object3D>();
        child->position.set(0, 0, 2);
        host->add(child);
        host->updateMatrixWorld(true);
        writeElements(output, "childWorld", child->matrixWorld);
        output.set("childNeedsUpdate", JSValue(child->matrixWorldNeedsUpdate));

        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

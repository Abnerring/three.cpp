#include "utils/GeometryUtils.h"

#include "core/JSValue.h"

#include <iostream>

namespace {
JSValue pointsToArray(const std::vector<Vector3>& points) {
    auto array = JSValue::array();
    std::size_t index = 0;
    for (const auto& point : points) {
        array.set(index++, JSValue(point.x));
        array.set(index++, JSValue(point.y));
        array.set(index++, JSValue(point.z));
    }
    return array;
}

JSValue numbersToArray(const std::vector<double>& numbers) {
    auto array = JSValue::array();
    std::size_t index = 0;
    for (const double number : numbers) array.set(index++, JSValue(number));
    return array;
}
}  // namespace

int main() {
    try {
        auto output = JSValue::object(nullptr);
        output.set("hilbert2DOne", pointsToArray(GeometryUtils::hilbert2D()));
        output.set("hilbert2DTwo", pointsToArray(GeometryUtils::hilbert2D(Vector3(0, 0, 0), 10, 2)));
        output.set("hilbert2DCustom",
                   pointsToArray(GeometryUtils::hilbert2D(Vector3(1, 2, 3), 4, 1, 3, 2, 1, 0)));
        output.set("hilbert3DOne", pointsToArray(GeometryUtils::hilbert3D()));
        output.set("hilbert3DCustom",
                   pointsToArray(GeometryUtils::hilbert3D(Vector3(1, 2, 3), 4, 1, 3, 2, 1, 0, 7, 6, 5, 4)));
        output.set("gosperOne", numbersToArray(GeometryUtils::gosper(1)));
        output.set("gosperTwo", numbersToArray(GeometryUtils::gosper(2)));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}

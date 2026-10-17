// Differential probe for the native src/math/Frustum.js port.
//
// Each stdin line is `op n11 ... n44 args...`, so the frustum is built from the same sixteen numbers on both
// sides and then queried. Sixteen numbers in set order rather than an element array, because that is the
// order both constructors take and it removes any question of storage convention from the comparison.
#include "core/JsonValue.h"
#include "math/Box3.h"
#include "math/Frustum.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include "math/Vector3.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
JsonValue encode(double value) {
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
double at(const std::vector<double>& values, std::size_t index) {
    return index < values.size() ? values[index] : 0.0;
}
Vector3 point(const std::vector<double>& values, std::size_t offset) {
    return Vector3(at(values, offset), at(values, offset + 1), at(values, offset + 2));
}
Frustum frustumOf(const std::vector<double>& values) {
    Matrix4 matrix;
    matrix.set(at(values, 0), at(values, 1), at(values, 2), at(values, 3),
               at(values, 4), at(values, 5), at(values, 6), at(values, 7),
               at(values, 8), at(values, 9), at(values, 10), at(values, 11),
               at(values, 12), at(values, 13), at(values, 14), at(values, 15));
    Frustum frustum;
    frustum.setFromProjectionMatrix(matrix);
    return frustum;
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        std::vector<double> values;
        double value = 0;
        while (fields >> value) values.push_back(value);
        const auto frustum = frustumOf(values);
        JsonValue result;
        if (op == "containsPoint") result = JsonValue(JsonValue::Object{{"value", frustum.containsPoint(point(values, 16))}});
        else if (op == "intersectsBox") result = JsonValue(JsonValue::Object{{"value", frustum.intersectsBox(Box3(point(values, 16), point(values, 19)))}});
        else if (op == "intersectsSphere") result = JsonValue(JsonValue::Object{{"value", frustum.intersectsSphere(Sphere(point(values, 16), at(values, 19)))}});
        else if (op == "planes") {
            JsonValue::Array constants;
            for (const auto& plane : frustum.planes) constants.emplace_back(encode(plane.constant));
            result = JsonValue(JsonValue::Object{{"constants", JsonValue(constants)}});
        } else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
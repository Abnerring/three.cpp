// Differential probe for the native src/math/Matrix4.js port.
//
// Each stdin line is `op numeric-args...`, parsed with strtod so NaN and Infinity arrive as named. What is
// compared is the 16 elements in storage order, or a scalar for the determinant, so a single transposed
// cell fails the case. Signed zero is spelled out, because a JSON number cannot carry it and inverting a
// matrix produces it.
#include "core/JsonValue.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
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
JsonValue matrixResult(const Matrix4& m) {
    JsonValue::Array elements;
    for (std::size_t index = 0; index < 16; ++index) elements.emplace_back(encode(m.elements()[index]));
    return JsonValue(JsonValue::Object{{"elements", JsonValue(elements)}});
}
Matrix4 fromArgs(const std::vector<double>& a, std::size_t offset) {
    Matrix4 m;
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t column = 0; column < 4; ++column)
            m.elements()[column * 4 + row] = offset + row * 4 + column < a.size() ? a[offset + row * 4 + column] : 0.0;
    return m;
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        std::vector<double> a;
        std::string token;
        while (fields >> token) a.push_back(std::strtod(token.c_str(), nullptr));
        const auto at = [&](std::size_t index) { return index < a.size() ? a[index] : 0.0; };
        JsonValue result;
        if (op == "determinant") result = JsonValue(JsonValue::Object{{"value", encode(fromArgs(a, 0).determinant())}});
        else if (op == "invert") result = matrixResult(Matrix4(fromArgs(a, 0)).invert());
        else if (op == "transpose") result = matrixResult(Matrix4(fromArgs(a, 0)).transpose());
        else if (op == "multiplyScalar") result = matrixResult(Matrix4(fromArgs(a, 0)).multiplyScalar(at(16)));
        else if (op == "compose") {
            const Vector3 position(at(0), at(1), at(2));
            const Quaternion quaternion(at(3), at(4), at(5), at(6));
            const Vector3 scale(at(7), at(8), at(9));
            Matrix4 m;
            m.compose(position, quaternion, scale);
            result = matrixResult(m);
        } else if (op == "decompose") {
            Vector3 position, scale;
            Quaternion quaternion;
            Matrix4(fromArgs(a, 0)).decompose(position, quaternion, scale);
            result = JsonValue(JsonValue::Object{
                {"px", encode(position.x)}, {"py", encode(position.y)}, {"pz", encode(position.z)},
                {"qx", encode(quaternion.x)}, {"qy", encode(quaternion.y)}, {"qz", encode(quaternion.z)}, {"qw", encode(quaternion.w)},
                {"sx", encode(scale.x)}, {"sy", encode(scale.y)}, {"sz", encode(scale.z)}});
        } else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
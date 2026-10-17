// Differential probe for the native src/math/Matrix3.js port.
//
// Each stdin line is `op n11 ... n33` for the nine element operations, or `op n11 ... n44` for the two that
// come from a four by four matrix. Elements are compared in storage order, so a single transposed cell
// fails, and signed zero is spelled out because inverting the identity produces it.
#include "core/JsonValue.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
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
JsonValue matrixResult(const Matrix3& m) {
    JsonValue::Array elements;
    for (std::size_t index = 0; index < 9; ++index) elements.emplace_back(encode(m.elements()[index]));
    return JsonValue(JsonValue::Object{{"elements", JsonValue(elements)}});
}
double at(const std::vector<double>& a, std::size_t index) { return index < a.size() ? a[index] : 0.0; }
Matrix3 fromNine(const std::vector<double>& a) {
    Matrix3 m;
    m.set(at(a, 0), at(a, 1), at(a, 2), at(a, 3), at(a, 4), at(a, 5), at(a, 6), at(a, 7), at(a, 8));
    return m;
}
Matrix4 fromSixteen(const std::vector<double>& a) {
    Matrix4 m;
    m.set(at(a, 0), at(a, 1), at(a, 2), at(a, 3), at(a, 4), at(a, 5), at(a, 6), at(a, 7),
          at(a, 8), at(a, 9), at(a, 10), at(a, 11), at(a, 12), at(a, 13), at(a, 14), at(a, 15));
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
        JsonValue result;
        if (op == "determinant") result = JsonValue(JsonValue::Object{{"value", encode(fromNine(a).determinant())}});
        else if (op == "invert") result = matrixResult(fromNine(a).invert());
        else if (op == "transpose") result = matrixResult(fromNine(a).transpose());
        else if (op == "multiplyScalar") result = matrixResult(fromNine(a).multiplyScalar(at(a, 9)));
        else if (op == "setFromMatrix4") result = matrixResult(Matrix3().setFromMatrix4(fromSixteen(a)));
        else if (op == "getNormalMatrix") result = matrixResult(Matrix3().getNormalMatrix(fromSixteen(a)));
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
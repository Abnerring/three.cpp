// Differential probe for the native src/math/Euler.js port.
//
// `euler.reorder FROM TO x y z` converts an Euler into another order; `euler.setFromQuaternion ORDER qx qy
// qz qw` and `euler.setFromRotationMatrix ORDER n11 ... n44` do the two conversions reorder goes through.
// The cases lean on the order permutations, the singular branch setFromQuaternion picks at gimbal lock,
// and signed zero, which a JSON number cannot carry.
#include "core/JsonValue.h"
#include "math/Euler.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
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
const char* nameOf(EulerOrder order) {
    switch (order) {
        case EulerOrder::XYZ: return "XYZ";
        case EulerOrder::YXZ: return "YXZ";
        case EulerOrder::ZXY: return "ZXY";
        case EulerOrder::ZYX: return "ZYX";
        case EulerOrder::YZX: return "YZX";
        case EulerOrder::XZY: return "XZY";
    }
    return "XYZ";
}
EulerOrder orderOf(const std::string& name) {
    if (name == "YXZ") return EulerOrder::YXZ;
    if (name == "ZXY") return EulerOrder::ZXY;
    if (name == "ZYX") return EulerOrder::ZYX;
    if (name == "YZX") return EulerOrder::YZX;
    if (name == "XZY") return EulerOrder::XZY;
    return EulerOrder::XYZ;
}
JsonValue eulerResult(const Euler& e) {
    return JsonValue(JsonValue::Object{{"x", encode(e.x)}, {"y", encode(e.y)}, {"z", encode(e.z)},
        {"order", nameOf(e.order)}});
}
double number(const std::vector<std::string>& tokens, std::size_t index) {
    return index < tokens.size() ? std::strtod(tokens[index].c_str(), nullptr) : 0.0;
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        std::vector<std::string> tokens;
        std::string token;
        while (fields >> token) tokens.push_back(token);
        JsonValue result;
        if (op == "euler.reorder" && tokens.size() >= 5) {
            const auto from = orderOf(tokens[0]);
            const auto to = orderOf(tokens[1]);
            Euler euler(number(tokens, 2), number(tokens, 3), number(tokens, 4), from);
            result = eulerResult(euler.reorder(to));
        } else if (op == "euler.setFromQuaternion" && tokens.size() >= 5) {
            const auto order = orderOf(tokens[0]);
            Euler euler(0, 0, 0, order);
            result = eulerResult(euler.setFromQuaternion(Quaternion(number(tokens, 1), number(tokens, 2), number(tokens, 3), number(tokens, 4))));
        } else if (op == "euler.setFromRotationMatrix" && tokens.size() >= 17) {
            const auto order = orderOf(tokens[0]);
            Matrix4 m;
            m.set(number(tokens, 1), number(tokens, 2), number(tokens, 3), number(tokens, 4),
                  number(tokens, 5), number(tokens, 6), number(tokens, 7), number(tokens, 8),
                  number(tokens, 9), number(tokens, 10), number(tokens, 11), number(tokens, 12),
                  number(tokens, 13), number(tokens, 14), number(tokens, 15), number(tokens, 16));
            Euler euler(0, 0, 0, order);
            result = eulerResult(euler.setFromRotationMatrix(m));
        } else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
// Differential probe for the native src/math/Quaternion.js port.
//
// Each stdin line is `op numeric-args...`. Arguments are parsed with strtod so NaN and Infinity arrive as
// the numbers they name. The cases lean on what a hand port drifts on: normalising a zero quaternion,
// the angle to itself, inverting a non-unit quaternion, and slerp across the antipodal pair, which is the
// branch that decides between interpolating and falling back.
#include "core/JsonValue.h"
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
    // Signed zero cannot survive a JSON number, and it is observable here: inverting the identity
    // produces -0 components, which are not the same value as 0.
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
JsonValue quaternionResult(const Quaternion& q) {
    return JsonValue::Object{{"x", encode(q.x)}, {"y", encode(q.y)}, {"z", encode(q.z)}, {"w", encode(q.w)}};
}
JsonValue scalarResult(double value) { return JsonValue::Object{{"value", encode(value)}}; }
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
        const Quaternion first(at(0), at(1), at(2), at(3));
        const Quaternion second(at(4), at(5), at(6), at(7));
        JsonValue result;
        if (op == "dot") result = scalarResult(first.dot(second));
        else if (op == "length") result = scalarResult(first.length());
        else if (op == "normalize") result = quaternionResult(Quaternion(first).normalize());
        else if (op == "multiply") result = quaternionResult(Quaternion(first).multiply(second));
        else if (op == "invert") result = quaternionResult(Quaternion(first).invert());
        else if (op == "conjugate") result = quaternionResult(Quaternion(first).conjugate());
        else if (op == "angleTo") result = scalarResult(first.angleTo(second));
        else if (op == "slerp") result = quaternionResult(Quaternion(first).slerp(second, at(8)));
        else if (op == "setFromAxisAngle") result = quaternionResult(Quaternion().setFromAxisAngle(Vector3(at(0), at(1), at(2)), at(3)));
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}

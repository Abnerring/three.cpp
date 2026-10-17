// Differential probe for the native src/math/Vector3.js port.
//
// Each stdin line is `op numeric-args...` and the probe prints the vector or scalar result. Non-finite
// components are spelled out, and the cases lean on the places a hand port drifts: normalising a zero
// vector, a zero-length angle, min and max with NaN, and the spherical conversion with a zero radius.
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
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
JsonValue vectorResult(const Vector3& v) {
    return JsonValue::Object{{"x", encode(v.x)}, {"y", encode(v.y)}, {"z", encode(v.z)}};
}
JsonValue scalarResult(double value) { return JsonValue::Object{{"value", encode(value)}}; }
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        // Tokens are parsed with strtod so that NaN and Infinity, which the stream extractor refuses,
        // arrive as the numbers they are named after.
        std::vector<double> a;
        std::string token;
        while (fields >> token) a.push_back(std::strtod(token.c_str(), nullptr));
        const auto at = [&](std::size_t index) { return index < a.size() ? a[index] : 0.0; };
        const Vector3 first(at(0), at(1), at(2));
        JsonValue result;
        if (op == "cross") result = vectorResult(Vector3().crossVectors(first, Vector3(at(3), at(4), at(5))));
        else if (op == "dot") result = scalarResult(first.dot(Vector3(at(3), at(4), at(5))));
        else if (op == "length") result = scalarResult(first.length());
        else if (op == "normalize") result = vectorResult(Vector3(first).normalize());
        else if (op == "lerp") result = vectorResult(Vector3(first).lerp(Vector3(at(3), at(4), at(5)), at(6)));
        else if (op == "angleTo") result = scalarResult(first.angleTo(Vector3(at(3), at(4), at(5))));
        else if (op == "distanceTo") result = scalarResult(first.distanceTo(Vector3(at(3), at(4), at(5))));
        else if (op == "clamp") result = vectorResult(Vector3(first).clamp(Vector3(at(3), at(4), at(5)), Vector3(at(6), at(7), at(8))));
        else if (op == "min") result = vectorResult(Vector3(first).min(Vector3(at(3), at(4), at(5))));
        else if (op == "max") result = vectorResult(Vector3(first).max(Vector3(at(3), at(4), at(5))));
        else if (op == "applyQuaternion") result = vectorResult(Vector3(first).applyQuaternion(Quaternion(at(3), at(4), at(5), at(6))));
        else if (op == "projectOnPlane") result = vectorResult(Vector3(first).projectOnPlane(Vector3(at(3), at(4), at(5))));
        else if (op == "reflect") result = vectorResult(Vector3(first).reflect(Vector3(at(3), at(4), at(5))));
        else if (op == "setFromSphericalCoords") result = vectorResult(Vector3().setFromSphericalCoords(at(0), at(1), at(2)));
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
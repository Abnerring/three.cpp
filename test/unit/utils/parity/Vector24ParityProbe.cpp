// Differential probe for the native src/math/Vector2.js and Vector4.js ports.
//
// Each stdin line is `v2.op args...` or `v4.op args...`. Arguments are parsed with strtod so NaN and
// Infinity arrive as named, and results spell out signed zero. The cases lean on the places a hand port
// drifts: normalising a zero vector, the angle and distance to itself, min and max with NaN, clamping a
// reversed range, and applying a matrix to a point rather than to a direction.
#include "core/JsonValue.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
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
JsonValue two(const Vector2& v) { return JsonValue(JsonValue::Object{{"x", encode(v.x)}, {"y", encode(v.y)}}); }
JsonValue four(const Vector4& v) {
    return JsonValue(JsonValue::Object{{"x", encode(v.x)}, {"y", encode(v.y)}, {"z", encode(v.z)}, {"w", encode(v.w)}});
}
JsonValue scalar(double value) { return JsonValue(JsonValue::Object{{"value", encode(value)}}); }
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
        if (op == "v2.dot") result = scalar(Vector2(at(0), at(1)).dot(Vector2(at(2), at(3))));
        else if (op == "v2.cross") result = scalar(Vector2(at(0), at(1)).cross(Vector2(at(2), at(3))));
        else if (op == "v2.length") result = scalar(Vector2(at(0), at(1)).length());
        else if (op == "v2.normalize") result = two(Vector2(at(0), at(1)).normalize());
        else if (op == "v2.angle") result = scalar(Vector2(at(0), at(1)).angle());
        else if (op == "v2.angleTo") result = scalar(Vector2(at(0), at(1)).angleTo(Vector2(at(2), at(3))));
        else if (op == "v2.distanceTo") result = scalar(Vector2(at(0), at(1)).distanceTo(Vector2(at(2), at(3))));
        else if (op == "v2.min") result = two(Vector2(at(0), at(1)).min(Vector2(at(2), at(3))));
        else if (op == "v2.max") result = two(Vector2(at(0), at(1)).max(Vector2(at(2), at(3))));
        else if (op == "v2.clamp") result = two(Vector2(at(0), at(1)).clamp(Vector2(at(2), at(3)), Vector2(at(4), at(5))));
        else if (op == "v2.lerp") result = two(Vector2(at(0), at(1)).lerp(Vector2(at(2), at(3)), at(4)));
        else if (op == "v4.dot") result = scalar(Vector4(at(0), at(1), at(2), at(3)).dot(Vector4(at(4), at(5), at(6), at(7))));
        else if (op == "v4.length") result = scalar(Vector4(at(0), at(1), at(2), at(3)).length());
        else if (op == "v4.normalize") result = four(Vector4(at(0), at(1), at(2), at(3)).normalize());
        else if (op == "v4.min") result = four(Vector4(at(0), at(1), at(2), at(3)).min(Vector4(at(4), at(5), at(6), at(7))));
        else if (op == "v4.max") result = four(Vector4(at(0), at(1), at(2), at(3)).max(Vector4(at(4), at(5), at(6), at(7))));
        else if (op == "v4.applyMatrix4") {
            Matrix4 m;
            m.set(at(4), at(5), at(6), at(7), at(8), at(9), at(10), at(11), at(12), at(13), at(14), at(15), at(16), at(17), at(18), at(19));
            result = four(Vector4(at(0), at(1), at(2), at(3)).applyMatrix4(m));
        } else if (op == "v4.lerp") result = four(Vector4(at(0), at(1), at(2), at(3)).lerp(Vector4(at(4), at(5), at(6), at(7)), at(8)));
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
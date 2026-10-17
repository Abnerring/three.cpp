// Differential probe for the native src/math/Plane.js port.
//
// Each stdin line is `op numeric-args...`, parsed with strtod so NaN and Infinity arrive as named. A plane
// reads a normal and a constant, then whatever the operation needs. Signed zero and the non-finite
// results are spelled out. applyMatrix4 takes sixteen numbers in set order, which is row major, so both
// sides build the matrix the same way rather than relying on storage order.
#include "core/JsonValue.h"
#include "math/Box3.h"
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
JsonValue pointResult(const Vector3& v) {
    return JsonValue(JsonValue::Object{{"x", encode(v.x)}, {"y", encode(v.y)}, {"z", encode(v.z)}});
}
JsonValue planeResult(const Plane& p) {
    return JsonValue(JsonValue::Object{{"normal", pointResult(p.normal)}, {"constant", encode(p.constant)}});
}
JsonValue scalarResult(double value) { return JsonValue(JsonValue::Object{{"value", encode(value)}}); }
JsonValue boolResult(bool value) { return JsonValue(JsonValue::Object{{"value", value}}); }
Vector3 point(const std::vector<double>& a, std::size_t offset) {
    const auto at = [&](std::size_t index) { return offset + index < a.size() ? a[offset + index] : 0.0; };
    return Vector3(at(0), at(1), at(2));
}
Plane planeOf(const std::vector<double>& a) { return Plane(point(a, 0), a.size() > 3 ? a[3] : 0.0); }
Matrix4 matrixOf(const std::vector<double>& a, std::size_t offset) {
    const auto at = [&](std::size_t index) { return offset + index < a.size() ? a[offset + index] : 0.0; };
    Matrix4 m;
    m.set(at(0), at(1), at(2), at(3), at(4), at(5), at(6), at(7),
          at(8), at(9), at(10), at(11), at(12), at(13), at(14), at(15));
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
        Vector3 target;
        JsonValue result;
        if (op == "plane.normalize") result = planeResult(Plane(planeOf(a)).normalize());
        else if (op == "plane.distance") result = scalarResult(planeOf(a).distanceToPoint(point(a, 4)));
        else if (op == "plane.project") result = pointResult(planeOf(a).projectPoint(point(a, 4), target));
        else if (op == "plane.negate") result = planeResult(Plane(planeOf(a)).negate());
        else if (op == "plane.translate") result = planeResult(Plane(planeOf(a)).translate(point(a, 4)));
        else if (op == "plane.setFromNormalAndCoplanar") result = planeResult(Plane().setFromNormalAndCoplanarPoint(point(a, 0), point(a, 3)));
        else if (op == "plane.applyMatrix4") result = planeResult(Plane(planeOf(a)).applyMatrix4(matrixOf(a, 4)));
        else if (op == "plane.intersectsBox") result = boolResult(planeOf(a).intersectsBox(Box3(point(a, 4), point(a, 7))));
        else if (op == "plane.intersectsSphere") result = boolResult(planeOf(a).intersectsSphere(Sphere(point(a, 4), a.size() > 7 ? a[7] : 0.0)));
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
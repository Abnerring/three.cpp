// Differential probe for the native src/math/Box3.js and src/math/Sphere.js ports.
//
// Each stdin line is `op numeric-args...`, parsed with strtod so NaN and Infinity arrive as named. Box
// operations read a min and a max, sphere operations a centre and a radius, and whatever the operation
// needs after that. Signed zero and the non-finite results are spelled out, because JSON numbers cannot
// carry them.
#include "core/JsonValue.h"
#include "math/Box3.h"
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
JsonValue boxResult(const Box3& box) {
    return JsonValue(JsonValue::Object{
        {"min", pointResult(box.min)}, {"max", pointResult(box.max)}});
}
JsonValue scalarResult(double value) { return JsonValue(JsonValue::Object{{"value", encode(value)}}); }
JsonValue boolResult(bool value) { return JsonValue(JsonValue::Object{{"value", value}}); }
Vector3 point(const std::vector<double>& a, std::size_t offset) {
    const auto at = [&](std::size_t index) { return offset + index < a.size() ? a[offset + index] : 0.0; };
    return Vector3(at(0), at(1), at(2));
}
Box3 boxOf(const std::vector<double>& a) {
    Box3 box;
    box.min = point(a, 0);
    box.max = point(a, 3);
    return box;
}
Sphere sphereOf(const std::vector<double>& a) {
    Sphere sphere;
    sphere.center = point(a, 0);
    sphere.radius = a.size() > 3 ? a[3] : 0.0;
    return sphere;
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
        if (op == "box.center") result = pointResult(boxOf(a).getCenter(target));
        else if (op == "box.size") result = pointResult(boxOf(a).getSize(target));
        else if (op == "box.contains") result = boolResult(boxOf(a).containsPoint(point(a, 6)));
        else if (op == "box.clamp") result = pointResult(boxOf(a).clampPoint(point(a, 6), target));
        else if (op == "box.distance") result = scalarResult(boxOf(a).distanceToPoint(point(a, 6)));
        else if (op == "box.intersects") result = boolResult(boxOf(a).intersectsBox(Box3(point(a, 6), point(a, 9))));
        else if (op == "box.expand") result = boxResult(boxOf(a).expandByPoint(point(a, 6)));
        else if (op == "box.intersect") result = boxResult(boxOf(a).intersect(Box3(point(a, 6), point(a, 9))));
        else if (op == "sphere.contains") result = boolResult(sphereOf(a).containsPoint(point(a, 4)));
        else if (op == "sphere.distance") result = scalarResult(sphereOf(a).distanceToPoint(point(a, 4)));
        else if (op == "sphere.clamp") result = pointResult(sphereOf(a).clampPoint(point(a, 4), target));
        else if (op == "sphere.intersectsSphere") result = boolResult(sphereOf(a).intersectsSphere(sphereOf(std::vector<double>(a.begin() + 4, a.end()))));
        else if (op == "sphere.intersectsBox") result = boolResult(sphereOf(a).intersectsBox(boxOf(std::vector<double>(a.begin() + 4, a.end()))));
        else if (op == "sphere.boundingBox") { Box3 bounds; sphereOf(a).getBoundingBox(bounds); result = boxResult(bounds); }
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}

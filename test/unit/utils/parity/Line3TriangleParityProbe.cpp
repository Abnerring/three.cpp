// Differential probe for the native src/math/Line3.js and Triangle.js ports.
//
// Each stdin line is `line3.op ...` or `tri.op ...`. A line reads a start and an end, a triangle reads three
// corners, and whatever the operation needs follows. Results are points, scalars or booleans, with signed
// zero and the non-finite values spelled out. The cases lean on what a hand port drifts on: a degenerate
// line where the projection parameter divides by zero, a point exactly on a corner, and a triangle whose
// winding decides the normal.
#include "core/JsonValue.h"
#include "math/Line3.h"
#include "math/Plane.h"
#include "math/Triangle.h"
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
JsonValue scalarResult(double value) { return JsonValue(JsonValue::Object{{"value", encode(value)}}); }
JsonValue boolResult(bool value) { return JsonValue(JsonValue::Object{{"value", value}}); }
double at(const std::vector<double>& a, std::size_t index) { return index < a.size() ? a[index] : 0.0; }
Vector3 point(const std::vector<double>& a, std::size_t offset) {
    return Vector3(at(a, offset), at(a, offset + 1), at(a, offset + 2));
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
        const Line3 line3(point(a, 0), point(a, 3));
        const Triangle triangle(point(a, 0), point(a, 3), point(a, 6));
        const auto p = point(a, 9);
        Vector3 target;
        JsonValue result;
        if (op == "line3.closestPointToPoint") result = pointResult(line3.closestPointToPoint(p, true, target));
        else if (op == "line3.delta") result = pointResult(line3.delta(target));
        else if (op == "line3.distance") result = scalarResult(line3.distance());
        else if (op == "line3.closestPointToPointParameter") result = scalarResult(line3.closestPointToPointParameter(p, true));
        else if (op == "line3.distanceSq") result = scalarResult(line3.distanceSq());
        else if (op == "tri.getNormal") result = pointResult(triangle.getNormal(target));
        else if (op == "tri.getArea") result = scalarResult(triangle.getArea());
        else if (op == "tri.getMidpoint") result = pointResult(triangle.getMidpoint(target));
        else if (op == "tri.containsPoint") result = boolResult(triangle.containsPoint(p));
        else if (op == "tri.closestPointToPoint") result = pointResult(triangle.closestPointToPoint(p, target));
        else if (op == "tri.getPlane") { Plane plane; triangle.getPlane(plane); result = pointResult(plane.normal); }
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
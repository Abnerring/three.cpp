// Differential probe for the native src/math/Spherical.js, Cylindrical.js and Box2.js ports.
//
// Each stdin line is `sph.op ...`, `cyl.op ...` or `box2.op ...`. A spherical or cylindrical reads three
// numbers, a box reads a min and a max, and whatever the operation needs follows. The cases lean on what a
// hand port drifts on: the pole where phi is zero and the azimuth is undefined, a zero radius, and a box
// whose min sits above its max.
#include "core/JsonValue.h"
#include "math/Box2.h"
#include "math/Cylindrical.h"
#include "math/Spherical.h"
#include "math/Vector2.h"
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
JsonValue two(const Vector2& v) { return JsonValue(JsonValue::Object{{"x", encode(v.x)}, {"y", encode(v.y)}}); }
JsonValue scalar(double value) { return JsonValue(JsonValue::Object{{"value", encode(value)}}); }
JsonValue flag(bool value) { return JsonValue(JsonValue::Object{{"value", value}}); }
double at(const std::vector<double>& a, std::size_t index) { return index < a.size() ? a[index] : 0.0; }
Vector2 point(const std::vector<double>& a, std::size_t offset) { return Vector2(at(a, offset), at(a, offset + 1)); }
Box2 boxOf(const std::vector<double>& a) { return Box2(point(a, 0), point(a, 2)); }
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
        Vector2 target;
        if (op == "sph.setFromVector3") {
            Spherical s; s.setFromVector3(Vector3(at(a, 0), at(a, 1), at(a, 2)));
            result = JsonValue(JsonValue::Object{{"radius", encode(s.radius)}, {"phi", encode(s.phi)}, {"theta", encode(s.theta)}});
        } else if (op == "sph.makeSafe") {
            Spherical s(at(a, 0), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
            s.radius = at(a, 0); s.phi = at(a, 1); s.theta = at(a, 2);
            s.makeSafe();
            result = JsonValue(JsonValue::Object{{"radius", encode(s.radius)}, {"phi", encode(s.phi)}, {"theta", encode(s.theta)}});
        } else if (op == "cyl.setFromVector3") {
            Cylindrical c; c.setFromVector3(Vector3(at(a, 0), at(a, 1), at(a, 2)));
            result = JsonValue(JsonValue::Object{{"radius", encode(c.radius)}, {"theta", encode(c.theta)}, {"y", encode(c.y)}});
        } else if (op == "box2.getCenter") result = two(boxOf(a).getCenter(target));
        else if (op == "box2.getSize") result = two(boxOf(a).getSize(target));
        else if (op == "box2.containsPoint") result = flag(boxOf(a).containsPoint(point(a, 4)));
        else if (op == "box2.clampPoint") result = two(boxOf(a).clampPoint(point(a, 4), target));
        else if (op == "box2.distanceToPoint") result = scalar(boxOf(a).distanceToPoint(point(a, 4)));
        else if (op == "box2.intersectsBox") result = flag(boxOf(a).intersectsBox(Box2(point(a, 4), point(a, 6))));
        else if (op == "box2.expandByPoint") result = two(boxOf(a).expandByPoint(point(a, 4)).max);
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}
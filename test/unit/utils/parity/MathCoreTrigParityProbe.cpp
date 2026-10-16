// Differential probe for the native trigonometric code paths of the core math modules.
//
// Every line replays one operation of the pinned three.js classes so the comparison covers
// the values that depend on Math.sin/cos/asin/acos/atan/atan2/tan: Euler <-> Quaternion,
// Matrix3/Matrix4 rotations, spherical and cylindrical conversion, Vector2 angles and the
// Vector4 axis-angle helpers. compare-math-core-trig.mjs builds the same values from the
// pinned checkout and compares them with assert.deepEqual (no tolerances).
//
// Protocol (one JSON object per line out):
//   Q  <order> <x> <y> <z>                        Quaternion.setFromEuler
//   QA <ax> <ay> <az> <angle>                     Quaternion.setFromAxisAngle
//   QM <m0..m15>                                  Quaternion.setFromRotationMatrix
//   QS <x1 y1 z1 w1> <x2 y2 z2 w2> <t>            Quaternion.slerpQuaternions
//   E  <m0..m15> <order>                          Euler.setFromRotationMatrix
//   ME <x> <y> <z> <order>                        Matrix4.makeRotationFromEuler
//   M4 <kind> <angle>                             Matrix4.makeRotationX/Y/Z/Axis (kind 0..3)
//   M3 <angle>                                    Matrix3.makeRotation
//   V3S <radius> <phi> <theta>                    Vector3.setFromSphericalCoords
//   V3C <radius> <theta> <y>                      Vector3.setFromCylindricalCoords
//   V3A <x1 y1 z1> <x2 y2 z2>                     Vector3.angleTo
//   V2G <x> <y>                                   Vector2.angle
//   SP <x> <y> <z>                                Spherical.setFromVector3
//   CY <x> <y> <z>                                Cylindrical.setFromVector3
//   V4Q <x> <y> <z> <w>                           Vector4.setAxisAngleFromQuaternion
#include "core/JsonValue.h"
#include "math/Cylindrical.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Spherical.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue vectorJSON(const Vector2& value) { return JsonValue::Array{number(value.x), number(value.y)}; }
JsonValue vectorJSON(const Vector3& value) { return JsonValue::Array{number(value.x), number(value.y), number(value.z)}; }
JsonValue quaternionJSON(const Quaternion& value) {
    return JsonValue::Array{number(value.x), number(value.y), number(value.z), number(value.w)};
}
JsonValue matrixJSON(const Matrix3& value) {
    JsonValue::Array result;
    for (const auto element : value.elements()) result.push_back(number(element));
    return result;
}
JsonValue matrixJSON(const Matrix4& value) {
    JsonValue::Array result;
    for (const auto element : value.elements()) result.push_back(number(element));
    return result;
}
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
EulerOrder order(const std::string& name) {
    if (name == "YXZ") return EulerOrder::YXZ;
    if (name == "ZXY") return EulerOrder::ZXY;
    if (name == "ZYX") return EulerOrder::ZYX;
    if (name == "YZX") return EulerOrder::YZX;
    if (name == "XZY") return EulerOrder::XZY;
    return EulerOrder::XYZ;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind;
        stream >> kind;
        std::string token;
        try {
            JsonValue output = JsonValue::Object{};
            if (kind == "Q") {
                stream >> token;
                const auto eulerOrder = order(token);
                double x = 0, y = 0, z = 0;
                stream >> x >> y >> z;
                Quaternion q;
                q.setFromEuler(Euler(x, y, z, eulerOrder));
                output["value"] = quaternionJSON(q);
            } else if (kind == "QA") {
                double x = 0, y = 0, z = 0, angle = 0;
                stream >> x >> y >> z >> angle;
                Quaternion q;
                q.setFromAxisAngle(Vector3(x, y, z), angle);
                output["value"] = quaternionJSON(q);
            } else if (kind == "QM") {
                std::vector<double> values(16);
                for (auto& value : values) stream >> value;
                Matrix4 m;
                std::copy(values.begin(), values.end(), m.elements().begin());
                Quaternion q;
                q.setFromRotationMatrix(m);
                output["value"] = quaternionJSON(q);
            } else if (kind == "QS") {
                double x1 = 0, y1 = 0, z1 = 0, w1 = 0, x2 = 0, y2 = 0, z2 = 0, w2 = 0, t = 0;
                stream >> x1 >> y1 >> z1 >> w1 >> x2 >> y2 >> z2 >> w2 >> t;
                Quaternion result;
                result.slerpQuaternions(Quaternion(x1, y1, z1, w1), Quaternion(x2, y2, z2, w2), t);
                output["value"] = quaternionJSON(result);
            } else if (kind == "E") {
                std::vector<double> values(16);
                for (auto& value : values) stream >> value;
                stream >> token;
                Matrix4 m;
                std::copy(values.begin(), values.end(), m.elements().begin());
                Euler euler(0, 0, 0, order(token));
                euler.setFromRotationMatrix(m);
                output["value"] = JsonValue::Array{number(euler.x), number(euler.y), number(euler.z)};
            } else if (kind == "ME") {
                double x = 0, y = 0, z = 0;
                stream >> x >> y >> z >> token;
                Matrix4 m;
                m.makeRotationFromEuler(Euler(x, y, z, order(token)));
                output["value"] = matrixJSON(m);
            } else if (kind == "M4") {
                double which = 0, angle = 0;
                stream >> which >> angle;
                Matrix4 m;
                if (which == 0) m.makeRotationX(angle);
                else if (which == 1) m.makeRotationY(angle);
                else if (which == 2) m.makeRotationZ(angle);
                else m.makeRotationAxis(Vector3(1, 2, 3).normalize(), angle);
                output["value"] = matrixJSON(m);
            } else if (kind == "M3") {
                double angle = 0;
                stream >> angle;
                Matrix3 m;
                m.makeRotation(angle);
                output["value"] = matrixJSON(m);
            } else if (kind == "V3S") {
                double radius = 0, phi = 0, theta = 0;
                stream >> radius >> phi >> theta;
                Vector3 v;
                v.setFromSphericalCoords(radius, phi, theta);
                output["value"] = vectorJSON(v);
            } else if (kind == "V3C") {
                double radius = 0, theta = 0, y = 0;
                stream >> radius >> theta >> y;
                Vector3 v;
                v.setFromCylindricalCoords(radius, theta, y);
                output["value"] = vectorJSON(v);
            } else if (kind == "V3A") {
                double x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
                stream >> x1 >> y1 >> z1 >> x2 >> y2 >> z2;
                output["value"] = number(Vector3(x1, y1, z1).angleTo(Vector3(x2, y2, z2)));

            } else if (kind == "V2G") {
                double x = 0, y = 0;
                stream >> x >> y;
                output["value"] = number(Vector2(x, y).angle());
            } else if (kind == "SP") {
                double x = 0, y = 0, z = 0;
                stream >> x >> y >> z;
                Spherical spherical;
                spherical.setFromVector3(Vector3(x, y, z));
                output["value"] = JsonValue::Array{number(spherical.radius), number(spherical.phi),
                    number(spherical.theta)};
            } else if (kind == "CY") {
                double x = 0, y = 0, z = 0;
                stream >> x >> y >> z;
                Cylindrical cylindrical;
                cylindrical.setFromVector3(Vector3(x, y, z));
                output["value"] = JsonValue::Array{number(cylindrical.radius), number(cylindrical.theta),
                    number(cylindrical.y)};
            } else if (kind == "V4Q") {
                double x = 0, y = 0, z = 0, w = 0;
                stream >> x >> y >> z >> w;
                Vector4 v;
                v.setAxisAngleFromQuaternion(Quaternion(x, y, z, w));
                output["value"] = JsonValue::Array{number(v.x), number(v.y), number(v.z), number(v.w)};
            } else {
                throw std::runtime_error("unknown scenario kind: " + kind);
            }
            std::cout << output.dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}

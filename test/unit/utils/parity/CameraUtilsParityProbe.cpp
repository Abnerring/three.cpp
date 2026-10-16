// Differential probe for the pinned examples/jsm/utils/CameraUtils.js port.
//
// Protocol, one scenario per stdin line:
//   F <eye x y z> <near> <far> <aspect> <estimate 0|1> <pa x y z> <pb x y z> <pc x y z>
//
// It builds a PerspectiveCamera at the eye position, calls CameraUtils::frameCorners and dumps
// the resulting fov, quaternion and the projection matrix with its inverse.
#include "core/JsonValue.h"
#include "cameras/PerspectiveCamera.h"
#include "math/Vector3.h"
#include "utils/CameraUtils.h"
#include <charconv>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
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
JsonValue matrixJSON(const Matrix4& matrix) {
    JsonValue::Array values;
    for (const auto element : matrix.elements()) values.push_back(number(element));
    return values;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            if (kind != "F") throw std::runtime_error("unknown scenario kind: " + kind);
            double values[16] = {0};
            for (auto& value : values) {
                stream >> token;
                value = parse(token);
            }
            // values: eye( 0..2 ), near, far, aspect, corners( 6..14 ), estimate ( 15 ).
            // The pinned scenario builds `new PerspectiveCamera( 50, aspect, near, far )`.
            PerspectiveCamera camera(50, values[5], values[3], values[4]);
            camera.position.set(values[0], values[1], values[2]);
            const Vector3 pa(values[6], values[7], values[8]);
            const Vector3 pb(values[9], values[10], values[11]);
            const Vector3 pc(values[12], values[13], values[14]);
            CameraUtils::frameCorners(camera, pa, pb, pc, values[15] != 0);
            std::cout << JsonValue(JsonValue::Object{{"fov", number(camera.fov)},
                                      {"quaternion", JsonValue::Array{number(camera.quaternion.x),
                                                         number(camera.quaternion.y), number(camera.quaternion.z),
                                                         number(camera.quaternion.w)}},
                                      {"projectionMatrix", matrixJSON(camera.projectionMatrix)},
                                      {"projectionMatrixInverse", matrixJSON(camera.projectionMatrixInverse)}})
                             .dump()
                      << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
#include "nodes/core/EquirectUV.h"

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

double EquirectUV::clampForAngle(double value) {
    if (value < -1.0) return -1.0;
    if (value > 1.0) return 1.0;
    return value;
}

EquirectUV::Coordinates EquirectUV::toCoordinates(double x, double y, double z) {
    Coordinates out;
    // The angle takes the third part first and the first second.
    out.u = std::atan2(z, x) * (1.0 / (kPi * 2.0)) + 0.5;
    // The vertical takes the second part, clamped first so that a direction slightly longer
    // than one still gives an angle rather than a not a number.
    out.v = std::asin(clampForAngle(y)) * (1.0 / kPi) + 0.5;
    return out;
}

EquirectUV::Direction EquirectUV::toDirection(double u, double v) {
    Direction out;
    const double theta = (u - 0.5) * (kPi * 2.0);
    const double phi = (v - 0.5) * kPi;
    const double cosPhi = std::cos(phi);
    out.x = cosPhi * std::cos(theta);
    out.y = std::sin(phi);
    out.z = cosPhi * std::sin(theta);
    return out;
}

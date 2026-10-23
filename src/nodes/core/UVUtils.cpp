#include "nodes/core/UVUtils.h"

#include <cmath>

UVUtils::ReplacementKind UVUtils::kindOf(bool isFunction) {
    return isFunction ? ReplacementKind::Function : ReplacementKind::Constant;
}

double UVUtils::answer(ReplacementKind kind, double functionAnswer, double constantValue) {
    // A constant ignores what it is asked and gives itself.
    return kind == ReplacementKind::Function ? functionAnswer : constantValue;
}

UVUtils::Pair UVUtils::rotate(const Pair& uv, double radians, const Pair& centre) {
    // The offset is turned and the centre added back.
    const double du = uv.u - centre.u;
    const double dv = uv.v - centre.v;
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    Pair out;
    out.u = du * c - dv * s + centre.u;
    out.v = du * s + dv * c + centre.v;
    return out;
}

double UVUtils::spherizeOffset(double squaredDistance, double strength) {
    // The fourth power of the distance is the squared distance squared, and it is multiplied by
    // the strength; the distance itself is applied by the caller, which makes the whole
    // displacement the fifth power.
    const double fourth = squaredDistance * squaredDistance;
    return fourth * strength;
}

UVUtils::Pair UVUtils::spherize(const Pair& uv, double strength, const Pair& centre) {
    const double du = uv.u - centre.u;
    const double dv = uv.v - centre.v;
    const double squared = du * du + dv * dv;
    const double offset = spherizeOffset(squared, strength);
    Pair out;
    out.u = uv.u + du * offset;
    out.v = uv.v + dv * offset;
    return out;
}

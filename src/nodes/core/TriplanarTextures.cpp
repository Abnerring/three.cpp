#include "nodes/core/TriplanarTextures.h"

#include <cmath>

double TriplanarTextures::weightSum(double nx, double ny, double nz) {
    // The parts absolute, then normalised, then added.
    const double ax = std::fabs(nx);
    const double ay = std::fabs(ny);
    const double az = std::fabs(nz);
    const double length = std::sqrt(ax * ax + ay * ay + az * az);
    if (length == 0.0) return 0.0;
    return (ax + ay + az) / length;
}

TriplanarTextures::Weights TriplanarTextures::weights(double nx, double ny, double nz) {
    const double ax = std::fabs(nx);
    const double ay = std::fabs(ny);
    const double az = std::fabs(nz);
    const double length = std::sqrt(ax * ax + ay * ay + az * az);
    Weights out;
    if (length == 0.0) return out;
    // Normalised, then divided by their own sum so that the three add to one.
    const double sum = (ax + ay + az) / length;
    out.x = (ax / length) / sum;
    out.y = (ay / length) / sum;
    out.z = (az / length) / sum;
    return out;
}

TriplanarTextures::Pair TriplanarTextures::sampleCoordinates(char axis, double px, double py,
                                                            double pz, double scale) {
    // The pairs run in a cycle: y z for the first axis, z x for the second, x y for the third.
    Pair out;
    if (axis == 'X') {
        out.u = py; out.v = pz;
    } else if (axis == 'Y') {
        out.u = pz; out.v = px;
    } else {
        out.u = px; out.v = py;
    }
    out.u *= scale;
    out.v *= scale;
    return out;
}

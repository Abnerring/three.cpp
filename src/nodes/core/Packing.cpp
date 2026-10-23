#include "nodes/core/Packing.h"

#include <cmath>

double Packing::packPart(double direction) {
    return direction * 0.5 + 0.5;
}

double Packing::unpackPart(double colour) {
    // Twice the value less one, which undoes the packing for a value inside the range.
    return colour * 2.0 - 1.0;
}

double Packing::saturate(double value) {
    // A clamp, so negative values and values above one are both brought to the ends.
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

double Packing::reconstructedZ(double x, double y) {
    // One less the squared length, clamped, then rooted.
    const double remainder = 1.0 - (x * x + y * y);
    return std::sqrt(saturate(remainder));
}

std::string Packing::packRenameWarning() {
    return "TSL: \"directionToColor()\" has been renamed to \"packNormalToRGB()\".";
}

std::string Packing::unpackRenameWarning() {
    return "TSL: \"colorToDirection()\" has been renamed to \"unpackRGBToNormal()\".";
}

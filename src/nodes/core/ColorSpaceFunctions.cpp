#include "nodes/core/ColorSpaceFunctions.h"

#include <cmath>

double ColorSpaceFunctions::mix(double powerPiece, double linePiece, double factor) {
    // The usual mix: the second piece as the flag rises and the first as it falls.
    return powerPiece * (1.0 - factor) + linePiece * factor;
}

double ColorSpaceFunctions::eotfFactor(double value) {
    // One when the colour is small enough for the straight line.
    return value <= kEotfThreshold ? 1.0 : 0.0;
}

double ColorSpaceFunctions::oetfFactor(double value) {
    return value <= kOetfThreshold ? 1.0 : 0.0;
}

double ColorSpaceFunctions::eotf(double value) {
    // The power piece first, the straight line second, so a small colour takes the line.
    const double powerPiece = std::pow(value * kEotfScale + kEotfOffset, kEotfExponent);
    const double linePiece = value * kEotfSlope;
    return mix(powerPiece, linePiece, eotfFactor(value));
}

double ColorSpaceFunctions::oetf(double value) {
    const double powerPiece = std::pow(value, kOetfExponent) * kOetfScale - kOetfOffset;
    const double linePiece = value * kOetfSlope;
    return mix(powerPiece, linePiece, oetfFactor(value));
}

bool ColorSpaceFunctions::exponentsAreReciprocal() {
    // The reciprocal of two point four is nought point four one six six six and so on, while
    // the constant holds five decimal places, so the two differ in the sixth.
    return kOetfExponent == 1.0 / kEotfExponent;
}

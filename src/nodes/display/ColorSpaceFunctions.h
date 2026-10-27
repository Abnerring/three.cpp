#pragma once

// Port of the two transfer functions in src/nodes/display/ColorSpaceFunctions.js at the pinned
// revision. Each is the usual piecewise curve written as a mix of its two pieces, and the order
// the two are given to the mix is the thing to notice: the power piece comes first and the
// straight line second, so the flag that says the colour is small selects the straight line.
#include "core/JSValue.h"
JSValue nativeColorSpaceFunctions();
JSValue nativeColorSpaceFunctionsBinding(const std::string& name);

class ColorSpaceFunctions {
  public:
    // The threshold below which the straight line applies, going each way.
    static constexpr double kEotfThreshold = 0.04045;
    static constexpr double kOetfThreshold = 0.0031308;

    // The straight line's slopes, which are one over twelve point nine two in both directions.
    static constexpr double kEotfSlope = 0.0773993808;
    static constexpr double kOetfSlope = 12.92;

    // The constants the power piece is built from, going from sRGB to linear.
    static constexpr double kEotfScale = 0.9478672986;
    static constexpr double kEotfOffset = 0.0521327014;
    static constexpr double kEotfExponent = 2.4;

    // The same going back. The exponent here is five decimal places of the true reciprocal
    // rather than the reciprocal itself, so the two directions are not exact inverses of one
    // another; the value is written out here so that the shortness is visible.
    static constexpr double kOetfScale = 1.055;
    static constexpr double kOetfOffset = 0.055;
    static constexpr double kOetfExponent = 0.41666;

    // One channel converted each way.
    static double eotf(double value);
    static double oetf(double value);

    // The flag each direction mixes on: one when the colour is small enough for the straight
    // line, which for the mix means the second argument is chosen.
    static double eotfFactor(double value);
    static double oetfFactor(double value);

    // The mix itself, written out so the order of the two pieces is visible rather than implied.
    static double mix(double powerPiece, double linePiece, double factor);

    // Whether the exponent used going back really is the reciprocal of the one used coming
    // forward, which it is not.
    static bool exponentsAreReciprocal();
};

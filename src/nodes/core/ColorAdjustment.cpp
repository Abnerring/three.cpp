#include "nodes/core/ColorAdjustment.h"

#include <cmath>

namespace {
constexpr double kOneOverSqrtThree = 0.5773502691896258;
}

double ColorAdjustment::luminance(double r, double g, double b) {
    // The usual weights; the exact figures belong to the colour space rather than to this module.
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double ColorAdjustment::saturation(double colour, double luminanceValue, double adjustment) {
    // A mix with the adjustment as the parameter, so one gives the colour back unchanged and
    // nought gives the luminance.
    const double mixed = luminanceValue * (1.0 - adjustment) + colour * adjustment;
    return mixed > 0.0 ? mixed : 0.0;
}

ColorAdjustment::Vibrance ColorAdjustment::vibranceAmounts(double r, double g, double b,
                                                           double adjustment) {
    Vibrance out;
    out.average = (r + g + b) / 3.0;
    // The largest of the three, taken two at a time.
    out.maxPart = r;
    if (g > out.maxPart) out.maxPart = g;
    if (b > out.maxPart) out.maxPart = b;
    // The distance scaled by the adjustment and multiplied by minus three.
    out.amount = (out.maxPart - out.average) * adjustment * -3.0;
    return out;
}

double ColorAdjustment::vibranceMix(double colour, double maxPart, double amount) {
    // The mix towards the largest part by the amount, held at nought from below.
    const double mixed = colour * (1.0 - amount) + maxPart * amount;
    return mixed > 0.0 ? mixed : 0.0;
}

bool ColorAdjustment::hueAxisIsExact() {
    return kHueAxis == kOneOverSqrtThree;
}

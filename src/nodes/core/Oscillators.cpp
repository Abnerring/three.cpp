#include "nodes/core/Oscillators.h"

#include <cmath>

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

double Oscillators::fract(double value) {
    // The value less the greatest whole number not above it, so a negative input comes out
    // inside the range instead of keeping its sign.
    return value - std::floor(value);
}

double Oscillators::sine(double time) {
    // Three quarters added before the angle, which starts the wave at nought rather than at a
    // half.
    const double angle = (time + kSineOffset) * kTwoPi;
    return std::sin(angle) * 0.5 + 0.5;
}

double Oscillators::square(double time) {
    // The rounded fractional part; a fractional part of exactly a half rounds up.
    return std::round(fract(time));
}

double Oscillators::triangle(double time) {
    // The half added before folding, then doubled and shifted, then folded to positive. The fold
    // is what makes the two halves of the wave meet without a step.
    const double folded = fract(time + kTriangleOffset) * 2.0 - 1.0;
    return std::fabs(folded);
}

double Oscillators::sawtooth(double time) {
    return fract(time);
}

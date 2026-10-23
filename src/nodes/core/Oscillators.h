#pragma once

// Port of the four wave functions in src/nodes/utils/Oscillators.js at the pinned revision. Each
// takes a time and answers a value between nought and one. Three of the four carry an offset
// whose job is to make the wave start where a caller would expect, and one of them is built out
// of a rounding that is easy to mistake for a step.
class Oscillators {
  public:
    // The fractional part, which is the value less the greatest whole number not above it. That
    // definition, rather than a remainder, is what makes a negative input come out positive: a
    // remainder would keep the sign and leave the wave below nought.
    static double fract(double value);

    // The sine wave, which starts at nought because of the three quarters it adds before the
    // angle.
    static double sine(double time);

    // The square wave, which is the rounded fractional part. Rounding rather than a step at a
    // half differs at that one point: the fractional part of exactly a half rounds up, so the
    // wave steps to one there.
    static double square(double time);

    // The triangle, which starts at nought because of the half it adds before folding.
    static double triangle(double time);

    // The sawtooth, which is the fractional part itself.
    static double sawtooth(double time);

    // The offset the sine adds before its angle, which is what puts its start at nought.
    static constexpr double kSineOffset = 0.75;
    // The offset the triangle adds before folding, for the same reason.
    static constexpr double kTriangleOffset = 0.5;
};

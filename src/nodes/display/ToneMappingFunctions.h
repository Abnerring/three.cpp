#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeToneMappingFunctions();
JSValue nativeToneMappingFunctionsBinding(const std::string& name);

// Port of three of the curves in src/nodes/display/ToneMappingFunctions.js at the pinned
// revision. Each takes a colour and an exposure and answers a colour brought into the range
// nought to one. The third carries six constants whose arrangement matters more than their
// values, so they are named here rather than buried in the expression.
class ToneMappingFunctions {
  public:
    // Exposure alone, held to the range. The plainest of the three, and the only one that does
    // nothing but multiply and clamp.
    static double linear(double colour, double exposure);

    // The colour divided by itself plus one, which approaches one but never reaches it; the
    // clamp is therefore belt and braces rather than load bearing.
    static double reinhard(double colour, double exposure);

    // The filmic curve by Hejl and Burgess-Dawson. A black offset is taken off first and
    // clamped at nought, then a quotient of two quadratics is raised to a power.
    static double cineon(double colour, double exposure);

    // The constants of the filmic curve. The two quadratics share the same multiplier but not
    // the same addend, and only the second takes an extra constant after the multiplication,
    // which is where a transcription is most likely to go wrong.
    static constexpr double kBlackOffset = 0.004;
    static constexpr double kSharedSlope = 6.2;
    static constexpr double kNumeratorAddend = 0.5;
    static constexpr double kDenominatorAddend = 1.7;
    static constexpr double kDenominatorOffset = 0.06;
    static constexpr double kPower = 2.2;
};

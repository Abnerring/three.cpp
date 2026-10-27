#include "nodes/display/ToneMappingFunctions.h"

#include <cmath>

namespace {
double heldToOne(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}
}  // namespace

double ToneMappingFunctions::linear(double colour, double exposure) {
    return heldToOne(colour * exposure);
}

double ToneMappingFunctions::reinhard(double colour, double exposure) {
    const double scaled = colour * exposure;
    return heldToOne(scaled / (scaled + 1.0));
}

double ToneMappingFunctions::cineon(double colour, double exposure) {
    // The black offset comes off and the answer may not go below nought.
    double c = colour * exposure - kBlackOffset;
    if (c < 0.0) c = 0.0;
    // Two quadratics: the same slope in both, different addends, and only the second takes the
    // offset after the multiplication.
    const double numerator = c * (c * kSharedSlope + kNumeratorAddend);
    const double denominator = c * (c * kSharedSlope + kDenominatorAddend) + kDenominatorOffset;
    return std::pow(numerator / denominator, kPower);
}

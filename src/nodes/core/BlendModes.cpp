#include "nodes/core/BlendModes.h"

namespace {
// The clamp both burn and dodge need, written once so the two callers cannot drift apart even
// though the source writes their calls with the arguments the other way round.
double clampToOne(double value) {
    if (value > 1.0) return 1.0;
    if (value < 0.0) return 0.0;
    return value;
}
}  // namespace

double BlendModes::burn(double base, double blend) {
    // One less the clamped quotient of the inverted base by the blend.
    return 1.0 - clampToOne((1.0 - base) / blend);
}

double BlendModes::dodge(double base, double blend) {
    // The clamped quotient of the base by the inverted blend.
    return clampToOne(base / (1.0 - blend));
}

double BlendModes::screen(double base, double blend) {
    // One less the product of the two inverted colours.
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

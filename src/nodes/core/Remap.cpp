#include "nodes/core/Remap.h"

double Remap::value(double node, double inLow, double inHigh, double outLow, double outHigh) {
    // Normalise to nought and one, then stretch and shift.
    const double t = (node - inLow) / (inHigh - inLow);
    return t * (outHigh - outLow) + outLow;
}

double Remap::clamped(double node, double inLow, double inHigh, double outLow, double outHigh) {
    double t = (node - inLow) / (inHigh - inLow);
    // The clamp is applied to the normalised value, before the stretch.
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * (outHigh - outLow) + outLow;
}

bool Remap::runsClampStep(bool argumentIsDefined) {
    // The test is whether the argument is there, not what it says.
    return argumentIsDefined;
}

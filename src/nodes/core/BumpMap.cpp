#include "nodes/core/BumpMap.h"

bool BumpMap::skipsBump(bool wireframeIsTrue) {
    // Only a material that says exactly true is skipped; one that says nothing is not.
    return wireframeIsTrue;
}

double BumpMap::bumpScale(bool hasScale, double scale) {
    // A scale that is present is used however small, because the test is against null and not
    // for truth.
    return hasScale ? scale : 1.0;
}

double BumpMap::determinant(double sigmaXDotR1) {
    // The determinant itself; the caller multiplies in the facing.
    return sigmaXDotR1;
}

double BumpMap::gradientSign(double determinantValue) {
    // The sign only: minus one, nought or one.
    if (determinantValue > 0.0) return 1.0;
    if (determinantValue < 0.0) return -1.0;
    return 0.0;
}

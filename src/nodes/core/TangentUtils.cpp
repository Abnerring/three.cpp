#include "nodes/core/TangentUtils.h"

#include <cmath>

double TangentUtils::determinant(double tDotT, double bDotB) {
    // The larger of the two squared lengths, not their product.
    return tDotT > bDotB ? tDotT : bDotB;
}

bool TangentUtils::degenerate(double determinantValue) {
    // Only exactly nought is degenerate; a tiny but non zero determinant still gets a scale.
    return determinantValue == 0.0;
}

double TangentUtils::scale(double tDotT, double bDotB) {
    const double det = determinant(tDotT, bDotB);
    // A determinant of nought gives nought rather than the inverse square root of nought, which
    // would be an infinity.
    if (degenerate(det)) return 0.0;
    return 1.0 / std::sqrt(det);
}

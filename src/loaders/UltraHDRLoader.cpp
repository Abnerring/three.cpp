#include "loaders/UltraHDRLoader.h"

#include <cmath>

namespace {
// The byte to index scale is one over two hundred and fifty five, and the offset moves the
// curve so that index zero sits at the linear value of black.
constexpr double kByteScale = 0.003717127;
constexpr double kByteOffset = 0.0521327014;
constexpr double kExponent = 2.4;
}  // namespace

double UltraHDRLoader::srgbToLinear(int index) {
    // An index past the table is clamped rather than reading out of bounds.
    const int clamped = index < 0 ? 0 : (index >= kTableSize ? kTableSize - 1 : index);
    return std::pow(clamped * kByteScale + kByteOffset, kExponent);
}

double UltraHDRLoader::gainMapBoost(double gainValue, double inverseGamma, bool gammaIsOne,
                                    double minBoost, double maxBoost) {
    // The map is stored as a byte, so the value arrives as nought through one.
    const double recovery =
        gammaIsOne ? gainValue : std::pow(gainValue, inverseGamma);
    // The boost itself interpolates in the log domain, which is why the endpoints are logs.
    return minBoost + (maxBoost - minBoost) * recovery;
}

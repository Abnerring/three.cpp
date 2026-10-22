#pragma once
#include <cstdint>

// Port of the arithmetic in examples/jsm/loaders/UltraHDRLoader.js at the pinned revision.
// Ultra HDR carries an ordinary eight bit image plus a gain map that says how much to boost
// each pixel; combining them needs the sRGB transfer function and a boost that interpolates
// between the minimum and maximum the file declares. Decoding the JPEG itself needs a codec
// the port does not carry, so only the arithmetic is portable.
class UltraHDRLoader {
  public:
    // How many entries the sRGB table holds.
    static constexpr int kTableSize = 1024;

    // The linear value of one table index. Everything below eight bit resolution is covered by
    // the table, so a byte is a multiple of four in it.
    static double srgbToLinear(int index);

    // The boost a gain map value asks for: the value scaled to nought through one, optionally
    // raised to a recovery exponent, then interpolated between the declared minimum and
    // maximum in the log domain.
    static double gainMapBoost(double gainValue, double inverseGamma, bool gammaIsOne,
                               double minBoost, double maxBoost);
};

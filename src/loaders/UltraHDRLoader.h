#pragma once
#include <cstdint>
#include <vector>

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

    // The gain map description carried by an ISO 21496-1 block. Values may be written over a
    // common denominator or as numerator and denominator pairs; both are reduced here to the
    // same numbers. Every field is big endian, unlike most of the other formats the port reads.
    struct IsoMetadata {
        bool backwardDirection = false;
        bool useCommonDenominator = false;
        double gainMapMin = 0;
        double gainMapMax = 0;
        double gamma = 1;
        double offsetSdr = 0;
        double offsetHdr = 0;
        double hdrCapacityMin = 0;
        double hdrCapacityMax = 0;
    };

    // Reads the block. Throws when it is too short for the fields its flags say are present.
    static IsoMetadata parseIsoMetadata(const std::vector<std::uint8_t>& buffer);
};

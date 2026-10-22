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
    // The linear value of one sRGB sample. Below about ten and a third the curve is a
    // straight line with a much smaller slope, between there and the table size it is a
    // table lookup truncated to an index, and above that the formula applies directly.
    static double srgbToLinear(double value);

    // The boost a gain map value asks for: the value scaled to nought through one, optionally
    // raised to a recovery exponent, then interpolated between the declared minimum and
    // maximum in the log domain.
    // Combines one sRGB sample with a gain map value. The boost is a base two exponent
    // scaled by the weight, where a zero product means no boost at all rather than a
    // factor of one; the result is then converted to linear and clamped to the largest
    // value a half float holds.
    static double composeHdr(double sdrValue, double gainBoost, double weight,
                             double offsetSdr, double offsetHdr);

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

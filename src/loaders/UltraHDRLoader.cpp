#include "loaders/UltraHDRLoader.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
// The byte to index scale is one over two hundred and fifty five, and the offset moves the
// curve so that index zero sits at the linear value of black.
constexpr double kByteScale = 0.003717127;
constexpr double kByteOffset = 0.0521327014;
constexpr double kExponent = 2.4;
}  // namespace


double UltraHDRLoader::gainMapBoost(double gainValue, double inverseGamma, bool gammaIsOne,
                                    double minBoost, double maxBoost) {
    // The map is stored as a byte, so the value arrives as nought through one.
    const double recovery =
        gammaIsOne ? gainValue : std::pow(gainValue, inverseGamma);
    // The boost itself interpolates in the log domain, which is why the endpoints are logs.
    return minBoost + (maxBoost - minBoost) * recovery;
}

namespace {
// Every field in this block is big endian.
std::uint32_t readUint32BE(const std::vector<std::uint8_t>& data, std::size_t at) {
    return (static_cast<std::uint32_t>(data[at]) << 24) |
           (static_cast<std::uint32_t>(data[at + 1]) << 16) |
           (static_cast<std::uint32_t>(data[at + 2]) << 8) | static_cast<std::uint32_t>(data[at + 3]);
}
std::int32_t readInt32BE(const std::vector<std::uint8_t>& data, std::size_t at) {
    return static_cast<std::int32_t>(readUint32BE(data, at));
}
}  // namespace

UltraHDRLoader::IsoMetadata UltraHDRLoader::parseIsoMetadata(
    const std::vector<std::uint8_t>& buffer) {
    IsoMetadata metadata;
    // Two bytes of minimum version and two of writer version are skipped.
    std::size_t offset = 4;
    if (buffer.size() < offset + 1)
        throw std::runtime_error("THREE.UltraHDRLoader: Invalid ISO metadata.");

    const std::uint8_t flags = buffer[offset++];
    metadata.backwardDirection = (flags & 0x4) != 0;
    metadata.useCommonDenominator = (flags & 0x8) != 0;

    if (metadata.useCommonDenominator) {
        // One denominator applies to every value.
        if (buffer.size() < offset + 24)
            throw std::runtime_error("THREE.UltraHDRLoader: Invalid ISO metadata.");
        const std::uint32_t denominator = readUint32BE(buffer, offset);
        offset += 4;
        const std::uint32_t baseHeadroom = readUint32BE(buffer, offset);
        offset += 4;
        metadata.hdrCapacityMin = std::log2(static_cast<double>(baseHeadroom) / denominator);
        const std::uint32_t alternateHeadroom = readUint32BE(buffer, offset);
        offset += 4;
        metadata.hdrCapacityMax = std::log2(static_cast<double>(alternateHeadroom) / denominator);

        if (buffer.size() < offset + 20)
            throw std::runtime_error("THREE.UltraHDRLoader: Invalid ISO metadata.");
        metadata.gainMapMin = readInt32BE(buffer, offset) / static_cast<double>(denominator);
        offset += 4;
        metadata.gainMapMax = readInt32BE(buffer, offset) / static_cast<double>(denominator);
        offset += 4;
        metadata.gamma = readUint32BE(buffer, offset) / static_cast<double>(denominator);
        offset += 4;
        // The two offsets are stored relative and scaled by the byte range.
        metadata.offsetSdr =
            readInt32BE(buffer, offset) / static_cast<double>(denominator) * 255.0;
        offset += 4;
        metadata.offsetHdr =
            readInt32BE(buffer, offset) / static_cast<double>(denominator) * 255.0;
        return metadata;
    }

    // Otherwise each value carries its own denominator, in numerator and denominator pairs.
    if (buffer.size() < offset + 8)
        throw std::runtime_error("THREE.UltraHDRLoader: Invalid ISO metadata.");
    const std::uint32_t baseHeadroomN = readUint32BE(buffer, offset);
    const std::uint32_t baseHeadroomD = readUint32BE(buffer, offset + 4);
    offset += 8;
    metadata.hdrCapacityMin = std::log2(static_cast<double>(baseHeadroomN) / baseHeadroomD);
    const std::uint32_t alternateHeadroomN = readUint32BE(buffer, offset);
    const std::uint32_t alternateHeadroomD = readUint32BE(buffer, offset + 4);
    offset += 8;
    metadata.hdrCapacityMax = std::log2(static_cast<double>(alternateHeadroomN) / alternateHeadroomD);

    const auto pair = [&buffer, &offset](bool signedNumerator) -> double {
        if (buffer.size() < offset + 8) return 0.0;
        // The cast is on each arm because a ternary between a signed and an unsigned type
        // takes the unsigned one and wraps a negative numerator.
        const double numerator =
            signedNumerator ? static_cast<double>(readInt32BE(buffer, offset))
                            : static_cast<double>(readUint32BE(buffer, offset));
        const std::uint32_t denominator = readUint32BE(buffer, offset + 4);
        offset += 8;
        return denominator == 0 ? 0.0 : numerator / denominator;
    };
    metadata.gainMapMin = pair(true);
    metadata.gainMapMax = pair(true);
    metadata.gamma = pair(false);
    metadata.offsetSdr = pair(true) * 255.0;
    metadata.offsetHdr = pair(true) * 255.0;
    return metadata;
}
double UltraHDRLoader::srgbToLinear(double value) {
    // The linear segment, which is a straight line with a slope far below the curve.
    if (value < 10.31475) return value * 0.000303527;
    // Inside the table range the value is truncated to an index, not interpolated.
    if (value < kTableSize) {
        const int index = static_cast<int>(value);
        return std::pow(index * kByteScale + kByteOffset, kExponent);
    }
    // Past the table the formula is evaluated directly.
    return std::pow(value * kByteScale + kByteOffset, kExponent);
}

double UltraHDRLoader::composeHdr(double sdrValue, double gainBoost, double weight,
                                  double offsetSdr, double offsetHdr) {
    // A zero product means the boost is off entirely rather than a factor of one.
    const double boost = gainBoost * weight;
    const double factor = boost == 0.0 ? 1.0 : std::pow(2.0, boost);
    const double hdrValue = (sdrValue + offsetSdr) * factor - offsetHdr;
    // The result is converted to linear and clamped to the half float maximum.
    const double linear = srgbToLinear(hdrValue);
    return std::min(std::max(linear, 0.0), 65504.0);
}

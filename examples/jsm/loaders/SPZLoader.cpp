#include "loaders/SPZLoader.h"

#include <cmath>
#include <limits>
#include <stdexcept>

std::int32_t SPZLoader::readInt24(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 3 > bytes.size()) return 0;
    // Shifting the top byte into the sign position and back sign extends, which is what the
    // upstream expression does.
    const std::uint32_t packed = (static_cast<std::uint32_t>(bytes[offset]) << 8) |
                                 (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
                                 (static_cast<std::uint32_t>(bytes[offset + 2]) << 24);
    return static_cast<std::int32_t>(packed) >> 8;
}

std::array<double, 4> SPZLoader::readXYZQuaternion(const std::vector<std::uint8_t>& bytes,
                                                   std::size_t offset) {
    std::array<double, 4> quaternion{0, 0, 0, 1};
    if (offset + 3 > bytes.size()) return quaternion;

    // A byte maps to the closed range minus one through one, one hundred and twenty seven and
    // a half being half a step below the maximum.
    const double x = bytes[offset] / 127.5 - 1.0;
    const double y = bytes[offset + 1] / 127.5 - 1.0;
    const double z = bytes[offset + 2] / 127.5 - 1.0;
    quaternion[0] = x;
    quaternion[1] = y;
    quaternion[2] = z;
    // The fourth component follows from unit length, clamped so rounding cannot make it
    // imaginary.
    quaternion[3] = std::sqrt(std::max(0.0, 1.0 - x * x - y * y - z * z));
    return quaternion;
}

double SPZLoader::quatComponent(int bits) {
    // The low nine bits are the magnitude as a fraction of the largest possible component, and
    // bit nine flips the sign.
    const double magnitude = 0.7071067811865476 * ((bits & 511) / 511.0);
    return (bits & 512) != 0 ? -magnitude : magnitude;
}

std::array<double, 4> SPZLoader::readSmallestThreeQuaternion(std::uint32_t packed) {
    // The top two bits say which component was the largest and so was left out.
    const unsigned largestIndex = packed >> 30;
    const double a = quatComponent(static_cast<int>(packed & 1023));
    const double b = quatComponent(static_cast<int>((packed >> 10) & 1023));
    const double c = quatComponent(static_cast<int>((packed >> 20) & 1023));

    // The three stored fields fill the remaining indices in descending order, so the low field
    // goes to the highest index left.
    std::array<double, 4> target{0, 0, 0, 0};
    switch (largestIndex) {
        case 0:
            target[1] = c;
            target[2] = b;
            target[3] = a;
            break;
        case 1:
            target[0] = c;
            target[2] = b;
            target[3] = a;
            break;
        case 2:
            target[0] = c;
            target[1] = b;
            target[3] = a;
            break;
        default:
            target[0] = c;
            target[1] = b;
            target[2] = a;
            break;
    }
    // The omitted component follows from unit length, clamped so rounding cannot make it
    // imaginary.
    target[largestIndex] = std::sqrt(std::max(0.0, 1.0 - (a * a + b * b + c * c)));
    return target;
}

double SPZLoader::scaleFromByte(int byte) {
    // A byte maps to a size on a geometric scale: sixteen steps per unit of exponent, offset by
    // ten so the range runs from about four hundredths of a unit up to two thousand.
    return std::exp(byte / 16.0 - 10.0);
}

double SPZLoader::fromHalfFloat(std::uint16_t half) {
    // The half float has a five bit exponent biased by fifteen and a ten bit fraction.
    const int sign = (half >> 15) & 0x1;
    const int exponent = (half >> 10) & 0x1f;
    const int fraction = half & 0x3ff;

    double value = 0.0;
    if (exponent == 0) {
        // A zero exponent with a fraction is a subnormal, scaled by two to the minus fourteen.
        value = std::ldexp(fraction / 1024.0, -14);
    } else if (exponent == 31) {
        // The largest exponent means infinity or, with a fraction, not a number.
        value = fraction == 0 ? std::numeric_limits<double>::infinity()
                              : std::numeric_limits<double>::quiet_NaN();
    } else {
        value = std::ldexp(1.0 + fraction / 1024.0, exponent - 15);
    }
    return sign ? -value : value;
}

std::vector<double> SPZLoader::readCenters(const std::vector<std::uint8_t>& bytes,
                                           std::size_t offset, int count, int version,
                                           int fractionalBits) {
    std::vector<double> centers;
    if (count <= 0) return centers;
    centers.reserve(static_cast<std::size_t>(count) * 3);

    if (version == 1) {
        // Three half floats per splat, two bytes each, little endian.
        for (int i = 0; i < count; ++i) {
            const std::size_t row = offset + static_cast<std::size_t>(i) * 6;
            for (int axis = 0; axis < 3; ++axis) {
                const std::size_t at = row + static_cast<std::size_t>(axis) * 2;
                if (at + 2 > bytes.size()) return centers;
                const std::uint16_t bits = static_cast<std::uint16_t>(
                    bytes[at] | (static_cast<std::uint16_t>(bytes[at + 1]) << 8));
                centers.push_back(fromHalfFloat(bits));
            }
        }
        return centers;
    }

    // A fixed point format: the scale is one over two to the fractional bits.
    const double fixedScale = 1.0 / static_cast<double>(1 << fractionalBits);
    for (int i = 0; i < count; ++i) {
        const std::size_t row = offset + static_cast<std::size_t>(i) * 9;
        for (int axis = 0; axis < 3; ++axis) {
            centers.push_back(readInt24(bytes, row + static_cast<std::size_t>(axis) * 3) * fixedScale);
        }
    }
    return centers;
}

double SPZLoader::colorLut(int byte) {
    // The spherical harmonic constant over a fifteenth, which is what turns the encoded colour
    // back into a linear value.
    const double scale = 0.2820947917738781 / 0.15;
    return ((byte / 255.0 - 0.5) * scale + 0.5) * 255.0;
}

int SPZLoader::degreeToVectors(int degree) {
    // A degree holds one more vector than the previous, and each band adds its own count.
    switch (degree) {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 8;
        case 3:
            return 15;
        default:
            return 15;
    }
}

int SPZLoader::harmonicsSourceStride(int degree) {
    // Three channels per vector.
    return degreeToVectors(degree) * 3;
}

void SPZLoader::requireUncompressed(std::uint8_t firstByte, std::uint8_t secondByte) {
    // The zstd frame magic starts with 0x28 0xb5, which is what a compressed file begins with.
    if (firstByte == 0x28 && secondByte == 0xb5)
        throw std::runtime_error("THREE.SPZLoader: Compressed SPZ is not supported");
}
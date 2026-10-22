#include "loaders/SPZLoader.h"

#include <cmath>

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
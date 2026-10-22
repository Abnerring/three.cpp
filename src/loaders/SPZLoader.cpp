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

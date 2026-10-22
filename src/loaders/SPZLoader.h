#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Port of the small readers in examples/jsm/loaders/SPZLoader.js at the pinned revision. SPZ
// holds Gaussian splats: compressed centres, a quaternion per splat and optional spherical
// harmonics colour. The file is zstd compressed, which the port does not carry, so only the
// byte level readers are portable and the container is reported rather than decoded.
class SPZLoader {
  public:
    // Reads a signed twenty four bit little endian integer. The shift left then arithmetic
    // shift right sign extends the top bit.
    static std::int32_t readInt24(const std::vector<std::uint8_t>& bytes, std::size_t offset);

    // Reads the xyz quaternion form: three signed bytes scaled to minus one through one, with
    // w reconstructed so the quaternion is unit length.
    static std::array<double, 4> readXYZQuaternion(const std::vector<std::uint8_t>& bytes,
                                                   std::size_t offset);

    // One ten bit quaternion component: bit nine is the sign and the low nine scale the
    // magnitude to the largest a component can take, one over the square root of two.
    static double quatComponent(int bits);

    // Reads the three component form: the top two bits name the largest component, the three
    // ten bit fields hold the others, and the largest follows from unit length. The fields fill
    // the remaining indices in descending order, so the low field lands on the highest index.
    static std::array<double, 4> readSmallestThreeQuaternion(std::uint32_t packed);

    // The scale table the format uses for Gaussian sizes: an exponential over a byte.
    static double scaleFromByte(int byte);
};

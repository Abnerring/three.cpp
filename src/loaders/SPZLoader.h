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
};

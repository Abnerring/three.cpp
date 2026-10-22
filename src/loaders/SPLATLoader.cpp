#include "loaders/SPLATLoader.h"

#include "loaders/GaussianSplatUtils.h"

#include <cstring>
#include <stdexcept>

namespace {
float readFloat32LE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    std::uint32_t bits = std::uint32_t(data[offset]) | (std::uint32_t(data[offset + 1]) << 8) |
                         (std::uint32_t(data[offset + 2]) << 16) |
                         (std::uint32_t(data[offset + 3]) << 24);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}
}  // namespace

SPLATLoader::Result SPLATLoader::parse(const std::vector<std::uint8_t>& buffer) const {
    if (buffer.size() % kRowSizeBytes != 0)
        throw std::runtime_error("THREE.SPLATLoader: Invalid .splat byte length.");

    const std::size_t count = buffer.size() / kRowSizeBytes;
    Result result;
    result.centers.assign(count * 3, 0.0f);
    result.covariances.assign(count * 6, 0.0f);
    result.colors.assign(count * 4, 0);

    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t rowOffset = i * kRowSizeBytes;
        // Unlike the MDD format, every read here passes the little endian flag.
        result.centers[i * 3 + 0] = readFloat32LE(buffer, rowOffset);
        result.centers[i * 3 + 1] = readFloat32LE(buffer, rowOffset + 4);
        result.centers[i * 3 + 2] = readFloat32LE(buffer, rowOffset + 8);

        const float sx = readFloat32LE(buffer, rowOffset + 12);
        const float sy = readFloat32LE(buffer, rowOffset + 16);
        const float sz = readFloat32LE(buffer, rowOffset + 20);

        result.colors[i * 4 + 0] = buffer[rowOffset + 24];
        result.colors[i * 4 + 1] = buffer[rowOffset + 25];
        result.colors[i * 4 + 2] = buffer[rowOffset + 26];
        result.colors[i * 4 + 3] = buffer[rowOffset + 27];

        // The quaternion is stored as w, x, y, z in bytes biased by one twenty eight.
        const double qw = (double(buffer[rowOffset + 28]) - 128.0) / 128.0;
        const double qx = (double(buffer[rowOffset + 29]) - 128.0) / 128.0;
        const double qy = (double(buffer[rowOffset + 30]) - 128.0) / 128.0;
        const double qz = (double(buffer[rowOffset + 31]) - 128.0) / 128.0;

        GaussianSplatUtils::writeCovariance(result.covariances.data(), i * 6, sx, sy, sz, qx, qy,
                                            qz, qw);
    }
    return result;
}

#include "loaders/MDDLoader.h"

#include "animation/tracks/NumberKeyframeTrack.h"

#include <cstring>
#include <stdexcept>

namespace {
std::uint32_t readUint32BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    if (offset + 4 > data.size())
        throw std::runtime_error("THREE.MDDLoader: Unexpected end of data");
    // The DataView reads upstream do not pass a little endian flag, so they are big endian.
    return (std::uint32_t(data[offset]) << 24) | (std::uint32_t(data[offset + 1]) << 16) |
           (std::uint32_t(data[offset + 2]) << 8) | std::uint32_t(data[offset + 3]);
}

float readFloat32BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    const std::uint32_t bits = readUint32BE(data, offset);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}
}  // namespace

MDDLoader::Result MDDLoader::parse(const std::vector<std::uint8_t>& data) const {
    const std::uint32_t totalFrames = readUint32BE(data, 0);
    const std::uint32_t totalPoints = readUint32BE(data, 4);
    std::size_t offset = 8;

    std::vector<float> times(totalFrames, 0.0f);
    // Upstream allocates a square array and walks its diagonal, which yields one animated
    // influence per frame and zeros elsewhere.
    std::vector<float> values(static_cast<std::size_t>(totalFrames) * totalFrames, 0.0f);

    for (std::uint32_t i = 0; i < totalFrames; ++i) {
        times[i] = readFloat32BE(data, offset);
        offset += 4;
        values[static_cast<std::size_t>(totalFrames) * i + i] = 1.0f;
    }

    auto track = std::make_shared<NumberKeyframeTrack>(JSValue(".morphTargetInfluences"),
                                                      AnimationArray(Float32Array(times)),
                                                      AnimationArray(Float32Array(values)));
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    tracks->push_back(track);
    const double duration = totalFrames > 0 ? double(times.back()) : 0.0;

    Result result;
    result.clip = std::make_shared<AnimationClip>(JSValue("default"), duration, tracks);
    result.morphTargets.reserve(totalFrames);

    for (std::uint32_t i = 0; i < totalFrames; ++i) {
        std::vector<float> morphTarget(static_cast<std::size_t>(totalPoints) * 3, 0.0f);
        for (std::uint32_t j = 0; j < totalPoints; ++j) {
            const std::size_t stride = std::size_t(j) * 3;
            morphTarget[stride + 0] = readFloat32BE(data, offset);
            offset += 4;
            morphTarget[stride + 1] = readFloat32BE(data, offset);
            offset += 4;
            morphTarget[stride + 2] = readFloat32BE(data, offset);
            offset += 4;
        }
        auto attribute = std::make_shared<TypedBufferAttribute<float>>(morphTarget, 3);
        attribute->name = "morph_" + std::to_string(i);
        result.morphTargets.push_back(attribute);
    }
    return result;
}

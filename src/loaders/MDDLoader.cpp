#include "loaders/MDDLoader.h"

#include <cstring>

namespace {
std::uint32_t readU32(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}
float readF32(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    const std::uint32_t bits = readU32(buffer, at);
    float value = 0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
}  // namespace

MDDLoader::Header MDDLoader::header(const std::vector<std::uint8_t>& buffer) {
    Header header;
    // A file too short to hold the two numbers has neither.
    if (buffer.size() < 8) return header;
    header.frames = readU32(buffer, 0);
    header.points = readU32(buffer, 4);
    return header;
}

std::size_t MDDLoader::timeCount(const Header& header) {
    return static_cast<std::size_t>(header.frames);
}

std::size_t MDDLoader::positionCount(const Header& header) {
    // Three numbers per point, with no padding between them.
    return static_cast<std::size_t>(header.points) * 3;
}

std::size_t MDDLoader::byteSize(const Header& header) {
    // The header, the times, then every frame's positions.
    const std::size_t times = timeCount(header) * 4;
    const std::size_t positions = static_cast<std::size_t>(header.frames) * positionCount(header) * 4;
    return 8 + times + positions;
}

std::vector<double> MDDLoader::times(const std::vector<std::uint8_t>& buffer, const Header& header) {
    std::vector<double> out;
    const std::size_t count = timeCount(header);
    if (buffer.size() < 8 + count * 4) return out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(static_cast<double>(readF32(buffer, 8 + i * 4)));
    }
    return out;
}

std::vector<double> MDDLoader::influences(const Header& header) {
    // A square matrix of zeros with a single one in each row.
    const std::size_t frames = static_cast<std::size_t>(header.frames);
    std::vector<double> values(frames * frames, 0.0);
    for (std::size_t i = 0; i < frames; ++i) {
        // Row i is offset by the frame count and its one sits at column i.
        values[frames * i + i] = 1.0;
    }
    return values;
}

std::string MDDLoader::morphName(std::uint32_t frame) {
    return "morph_" + std::to_string(frame);
}

std::vector<double> MDDLoader::framePositions(const std::vector<std::uint8_t>& buffer,
                                              const Header& header, std::uint32_t frame) {
    std::vector<double> out;
    const std::size_t count = positionCount(header);
    const std::size_t start = 8 + timeCount(header) * 4 + static_cast<std::size_t>(frame) * count * 4;
    if (frame >= header.frames) return out;
    if (start + count * 4 > buffer.size()) return out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(static_cast<double>(readF32(buffer, start + i * 4)));
    }
    return out;
}

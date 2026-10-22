#include "loaders/MD2Loader.h"

#include <cstring>
#include <stdexcept>

namespace {
std::int32_t readInt32LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 4 > data.size()) throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
    // Little endian: the lowest address holds the least significant byte.
    const std::uint32_t raw = std::uint32_t(data[at]) | (std::uint32_t(data[at + 1]) << 8) |
                              (std::uint32_t(data[at + 2]) << 16) |
                              (std::uint32_t(data[at + 3]) << 24);
    return static_cast<std::int32_t>(raw);
}

float readFloat32LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 4 > data.size()) throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
    const std::uint32_t bits = std::uint32_t(data[at]) | (std::uint32_t(data[at + 1]) << 8) |
                               (std::uint32_t(data[at + 2]) << 16) |
                               (std::uint32_t(data[at + 3]) << 24);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

std::uint16_t readUint16LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 2 > data.size()) throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
    return std::uint16_t(std::uint16_t(data[at]) | (std::uint16_t(data[at + 1]) << 8));
}

std::int16_t readInt16LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 2 > data.size()) throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
    const std::uint16_t raw = std::uint16_t(data[at]) | (std::uint16_t(data[at + 1]) << 8);
    return static_cast<std::int16_t>(raw);
}
}  // namespace

MD2Loader::Header MD2Loader::parseHeader(const std::vector<std::uint8_t>& buffer) {
    Header header;
    // The fields are read in this exact order, four bytes each.
    std::int32_t* fields[] = {&header.ident,         &header.version,      &header.skinwidth,
                              &header.skinheight,    &header.framesize,    &header.num_skins,
                              &header.num_vertices,  &header.num_st,       &header.num_tris,
                              &header.num_glcmds,    &header.num_frames,   &header.offset_skins,
                              &header.offset_st,     &header.offset_tris,  &header.offset_frames,
                              &header.offset_glcmds, &header.offset_end};
    for (std::size_t i = 0; i < 17; ++i) *fields[i] = readInt32LE(buffer, i * 4);
    return header;
}

bool MD2Loader::isValid(const Header& header) {
    return header.ident == kIdent && header.version == kVersion;
}

bool MD2Loader::isConsistent(const Header& header, std::size_t byteLength) {
    return static_cast<std::size_t>(header.offset_end) == byteLength;
}

std::vector<double> MD2Loader::parseUVs(const std::vector<std::uint8_t>& buffer,
                                        const Header& header) {
    std::vector<double> uvs;
    uvs.reserve(static_cast<std::size_t>(header.num_st) * 2);
    std::size_t offset = static_cast<std::size_t>(header.offset_st);
    for (std::int32_t i = 0; i < header.num_st; ++i) {
        const std::int16_t u = readInt16LE(buffer, offset + 0);
        const std::int16_t v = readInt16LE(buffer, offset + 2);
        // u is normalised by the skin width and v is flipped, so the origin is bottom left.
        uvs.push_back(static_cast<double>(u) / header.skinwidth);
        uvs.push_back(1.0 - static_cast<double>(v) / header.skinheight);
        offset += 4;
    }
    return uvs;
}

std::vector<MD2Loader::Triangle> MD2Loader::parseTriangles(const std::vector<std::uint8_t>& buffer,
                                                           const Header& header) {
    std::vector<Triangle> triangles;
    triangles.reserve(static_cast<std::size_t>(header.num_tris));
    std::size_t offset = static_cast<std::size_t>(header.offset_tris);
    for (std::int32_t i = 0; i < header.num_tris; ++i) {
        Triangle triangle;
        // Three vertex indices then three texture coordinate indices.
        for (int k = 0; k < 3; ++k) triangle.vertex[k] = readUint16LE(buffer, offset + k * 2);
        for (int k = 0; k < 3; ++k) triangle.uv[k] = readUint16LE(buffer, offset + 6 + k * 2);
        triangles.push_back(triangle);
        offset += 12;
    }
    return triangles;
}

std::vector<MD2Loader::FrameHeader> MD2Loader::parseFrameHeaders(
    const std::vector<std::uint8_t>& buffer, const Header& header) {
    std::vector<FrameHeader> frames;
    frames.reserve(static_cast<std::size_t>(header.num_frames));
    std::size_t offset = static_cast<std::size_t>(header.offset_frames);
    for (std::int32_t i = 0; i < header.num_frames; ++i) {
        FrameHeader frame;
        for (int k = 0; k < 3; ++k) frame.scale[k] = readFloat32LE(buffer, offset + k * 4);
        for (int k = 0; k < 3; ++k) frame.translation[k] = readFloat32LE(buffer, offset + 12 + k * 4);
        // The name is sixteen bytes and ends at the first zero.
        for (int k = 0; k < 16; ++k) {
            if (offset + 24 + static_cast<std::size_t>(k) >= buffer.size())
                throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
            const std::uint8_t character = buffer[offset + 24 + static_cast<std::size_t>(k)];
            if (character == 0) break;
            frame.name.push_back(static_cast<char>(character));
        }
        frames.push_back(frame);
        // Advance past the header and the per vertex data so the next frame lands right.
        offset += 24 + 16;
        offset += static_cast<std::size_t>(header.num_vertices) * 4;
    }
    return frames;
}
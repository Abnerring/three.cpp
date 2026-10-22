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

const std::array<std::array<double, 3>, 162>& MD2Loader::normalTable() {
    static const std::array<std::array<double, 3>, 162> table{{
    {-0.525731, 0.000000, 0.850651},
    {-0.442863, 0.238856, 0.864188},
    {-0.295242, 0.000000, 0.955423},
    {-0.309017, 0.500000, 0.809017},
    {-0.162460, 0.262866, 0.951056},
    {0.000000, 0.000000, 1.000000},
    {0.000000, 0.850651, 0.525731},
    {-0.147621, 0.716567, 0.681718},
    {0.147621, 0.716567, 0.681718},
    {0.000000, 0.525731, 0.850651},
    {0.309017, 0.500000, 0.809017},
    {0.525731, 0.000000, 0.850651},
    {0.295242, 0.000000, 0.955423},
    {0.442863, 0.238856, 0.864188},
    {0.162460, 0.262866, 0.951056},
    {-0.681718, 0.147621, 0.716567},
    {-0.809017, 0.309017, 0.500000},
    {-0.587785, 0.425325, 0.688191},
    {-0.850651, 0.525731, 0.000000},
    {-0.864188, 0.442863, 0.238856},
    {-0.716567, 0.681718, 0.147621},
    {-0.688191, 0.587785, 0.425325},
    {-0.500000, 0.809017, 0.309017},
    {-0.238856, 0.864188, 0.442863},
    {-0.425325, 0.688191, 0.587785},
    {-0.716567, 0.681718, -0.147621},
    {-0.500000, 0.809017, -0.309017},
    {-0.525731, 0.850651, 0.000000},
    {0.000000, 0.850651, -0.525731},
    {-0.238856, 0.864188, -0.442863},
    {0.000000, 0.955423, -0.295242},
    {-0.262866, 0.951056, -0.162460},
    {0.000000, 1.000000, 0.000000},
    {0.000000, 0.955423, 0.295242},
    {-0.262866, 0.951056, 0.162460},
    {0.238856, 0.864188, 0.442863},
    {0.262866, 0.951056, 0.162460},
    {0.500000, 0.809017, 0.309017},
    {0.238856, 0.864188, -0.442863},
    {0.262866, 0.951056, -0.162460},
    {0.500000, 0.809017, -0.309017},
    {0.850651, 0.525731, 0.000000},
    {0.716567, 0.681718, 0.147621},
    {0.716567, 0.681718, -0.147621},
    {0.525731, 0.850651, 0.000000},
    {0.425325, 0.688191, 0.587785},
    {0.864188, 0.442863, 0.238856},
    {0.688191, 0.587785, 0.425325},
    {0.809017, 0.309017, 0.500000},
    {0.681718, 0.147621, 0.716567},
    {0.587785, 0.425325, 0.688191},
    {0.955423, 0.295242, 0.000000},
    {1.000000, 0.000000, 0.000000},
    {0.951056, 0.162460, 0.262866},
    {0.850651, -0.525731, 0.000000},
    {0.955423, -0.295242, 0.000000},
    {0.864188, -0.442863, 0.238856},
    {0.951056, -0.162460, 0.262866},
    {0.809017, -0.309017, 0.500000},
    {0.681718, -0.147621, 0.716567},
    {0.850651, 0.000000, 0.525731},
    {0.864188, 0.442863, -0.238856},
    {0.809017, 0.309017, -0.500000},
    {0.951056, 0.162460, -0.262866},
    {0.525731, 0.000000, -0.850651},
    {0.681718, 0.147621, -0.716567},
    {0.681718, -0.147621, -0.716567},
    {0.850651, 0.000000, -0.525731},
    {0.809017, -0.309017, -0.500000},
    {0.864188, -0.442863, -0.238856},
    {0.951056, -0.162460, -0.262866},
    {0.147621, 0.716567, -0.681718},
    {0.309017, 0.500000, -0.809017},
    {0.425325, 0.688191, -0.587785},
    {0.442863, 0.238856, -0.864188},
    {0.587785, 0.425325, -0.688191},
    {0.688191, 0.587785, -0.425325},
    {-0.147621, 0.716567, -0.681718},
    {-0.309017, 0.500000, -0.809017},
    {0.000000, 0.525731, -0.850651},
    {-0.525731, 0.000000, -0.850651},
    {-0.442863, 0.238856, -0.864188},
    {-0.295242, 0.000000, -0.955423},
    {-0.162460, 0.262866, -0.951056},
    {0.000000, 0.000000, -1.000000},
    {0.295242, 0.000000, -0.955423},
    {0.162460, 0.262866, -0.951056},
    {-0.442863, -0.238856, -0.864188},
    {-0.309017, -0.500000, -0.809017},
    {-0.162460, -0.262866, -0.951056},
    {0.000000, -0.850651, -0.525731},
    {-0.147621, -0.716567, -0.681718},
    {0.147621, -0.716567, -0.681718},
    {0.000000, -0.525731, -0.850651},
    {0.309017, -0.500000, -0.809017},
    {0.442863, -0.238856, -0.864188},
    {0.162460, -0.262866, -0.951056},
    {0.238856, -0.864188, -0.442863},
    {0.500000, -0.809017, -0.309017},
    {0.425325, -0.688191, -0.587785},
    {0.716567, -0.681718, -0.147621},
    {0.688191, -0.587785, -0.425325},
    {0.587785, -0.425325, -0.688191},
    {0.000000, -0.955423, -0.295242},
    {0.000000, -1.000000, 0.000000},
    {0.262866, -0.951056, -0.162460},
    {0.000000, -0.850651, 0.525731},
    {0.000000, -0.955423, 0.295242},
    {0.238856, -0.864188, 0.442863},
    {0.262866, -0.951056, 0.162460},
    {0.500000, -0.809017, 0.309017},
    {0.716567, -0.681718, 0.147621},
    {0.525731, -0.850651, 0.000000},
    {-0.238856, -0.864188, -0.442863},
    {-0.500000, -0.809017, -0.309017},
    {-0.262866, -0.951056, -0.162460},
    {-0.850651, -0.525731, 0.000000},
    {-0.716567, -0.681718, -0.147621},
    {-0.716567, -0.681718, 0.147621},
    {-0.525731, -0.850651, 0.000000},
    {-0.500000, -0.809017, 0.309017},
    {-0.238856, -0.864188, 0.442863},
    {-0.262866, -0.951056, 0.162460},
    {-0.864188, -0.442863, 0.238856},
    {-0.809017, -0.309017, 0.500000},
    {-0.688191, -0.587785, 0.425325},
    {-0.681718, -0.147621, 0.716567},
    {-0.442863, -0.238856, 0.864188},
    {-0.587785, -0.425325, 0.688191},
    {-0.309017, -0.500000, 0.809017},
    {-0.147621, -0.716567, 0.681718},
    {-0.425325, -0.688191, 0.587785},
    {-0.162460, -0.262866, 0.951056},
    {0.442863, -0.238856, 0.864188},
    {0.162460, -0.262866, 0.951056},
    {0.309017, -0.500000, 0.809017},
    {0.147621, -0.716567, 0.681718},
    {0.000000, -0.525731, 0.850651},
    {0.425325, -0.688191, 0.587785},
    {0.587785, -0.425325, 0.688191},
    {0.688191, -0.587785, 0.425325},
    {-0.955423, 0.295242, 0.000000},
    {-0.951056, 0.162460, 0.262866},
    {-1.000000, 0.000000, 0.000000},
    {-0.850651, 0.000000, 0.525731},
    {-0.955423, -0.295242, 0.000000},
    {-0.951056, -0.162460, 0.262866},
    {-0.864188, 0.442863, -0.238856},
    {-0.951056, 0.162460, -0.262866},
    {-0.809017, 0.309017, -0.500000},
    {-0.864188, -0.442863, -0.238856},
    {-0.951056, -0.162460, -0.262866},
    {-0.809017, -0.309017, -0.500000},
    {-0.681718, 0.147621, -0.716567},
    {-0.681718, -0.147621, -0.716567},
    {-0.850651, 0.000000, -0.525731},
    {-0.688191, 0.587785, -0.425325},
    {-0.587785, 0.425325, -0.688191},
    {-0.425325, 0.688191, -0.587785},
    {-0.425325, -0.688191, -0.587785},
    {-0.587785, -0.425325, -0.688191},
    {-0.688191, -0.587785, -0.425325},
    }};
    return table;
}

std::size_t MD2Loader::decodeVertices(const std::vector<std::uint8_t>& buffer, std::size_t at,
                                      int count, const double scale[3],
                                      const double translation[3], std::vector<Vertex>& out) {
    std::size_t offset = at;
    for (int i = 0; i < count; ++i) {
        if (offset + 4 > buffer.size())
            throw std::runtime_error("THREE.MD2Loader: Unexpected end of data");
        const double x = buffer[offset + 0] * scale[0] + translation[0];
        const double y = buffer[offset + 1] * scale[1] + translation[1];
        const double z = buffer[offset + 2] * scale[2] + translation[2];
        const auto& normal = normalTable()[buffer[offset + 3]];

        Vertex vertex;
        // Upstream swaps the last two axes so the model is Y-up.
        vertex.position[0] = x;
        vertex.position[1] = z;
        vertex.position[2] = y;
        vertex.normal[0] = normal[0];
        vertex.normal[1] = normal[2];
        vertex.normal[2] = normal[1];
        out.push_back(vertex);
        offset += 4;
    }
    return offset - at;
}

MD2Loader::MorphTargets MD2Loader::buildMorphTargets(const std::vector<Frame>& frames,
                                                      const std::vector<Triangle>& triangles) {
    MorphTargets targets;
    // The triangle list flattened into a vertex index order, which is what the attributes use.
    std::vector<std::uint16_t> vertexOrder;
    vertexOrder.reserve(triangles.size() * 3);
    for (const auto& triangle : triangles)
        for (int k = 0; k < 3; ++k) vertexOrder.push_back(triangle.vertex[k]);

    for (const auto& frame : frames) {
        if (frame.vertices.empty()) continue;

        std::vector<double> positions;
        std::vector<double> normals;
        positions.reserve(vertexOrder.size() * 3);
        normals.reserve(vertexOrder.size() * 3);
        for (const auto index : vertexOrder) {
            const auto at = static_cast<std::size_t>(index);
            if (at >= frame.vertices.size()) {
                // A frame that does not cover the triangle stays short, as upstream would.
                positions.insert(positions.end(), {0.0, 0.0, 0.0});
                normals.insert(normals.end(), {0.0, 0.0, 0.0});
                continue;
            }
            const auto& vertex = frame.vertices[at];
            positions.insert(positions.end(),
                             {vertex.position[0], vertex.position[1], vertex.position[2]});
            normals.insert(normals.end(), {vertex.normal[0], vertex.normal[1], vertex.normal[2]});
        }
        // Both streams carry the frame name, as upstream sets attribute.name from it.
        targets.positions.push_back(std::move(positions));
        targets.normals.push_back(std::move(normals));
        targets.names.push_back(frame.header.name);
    }
    return targets;
}
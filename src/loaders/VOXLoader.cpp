#include "loaders/VOXLoader.h"

#include "math/Color.h"
#include "math/ColorManagement.h"

#include <stdexcept>

VOXLoader::Matrix VOXLoader::rotationMatrix(std::uint8_t byte) {
    // The low two bits give the column of the first row and the next two the second row.
    const int index1 = byte & 0x3;
    const int index2 = (byte >> 2) & 0x3;
    // The next three bits are the signs of each row.
    const double sign1 = ((byte >> 4) & 0x1) ? -1.0 : 1.0;
    const double sign2 = ((byte >> 5) & 0x1) ? -1.0 : 1.0;
    const double sign3 = ((byte >> 6) & 0x1) ? -1.0 : 1.0;

    if (index1 == index2)
        throw std::runtime_error("THREE.VOXLoader: Invalid rotation encoding");
    // Each row has one non zero entry, so the third row takes the column the others did not.
    const int index3 = 3 - index1 - index2;

    double r[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    r[0][index1] = sign1;
    r[1][index2] = sign2;
    r[2][index3] = sign3;

    // Change of basis from the VOX frame, where Z is up, to the port's Y up frame: the new
    // y is the old z and the new z is the negated old y.
    return {r[0][0], r[0][2], -r[0][1], 0.0,
            r[2][0], r[2][2], -r[2][1], 0.0,
            -r[1][0], -r[1][2], r[1][1], 0.0,
            0.0, 0.0, 0.0, 1.0};
}
namespace {
std::uint32_t readUint32LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    return static_cast<std::uint32_t>(data[at]) | (static_cast<std::uint32_t>(data[at + 1]) << 8) |
           (static_cast<std::uint32_t>(data[at + 2]) << 16) |
           (static_cast<std::uint32_t>(data[at + 3]) << 24);
}

// Reads the chunk list that starts at cursor and runs until end.
std::vector<VOXLoader::Chunk> readChunks(const std::vector<std::uint8_t>& data, std::size_t cursor,
                                         std::size_t end) {
    std::vector<VOXLoader::Chunk> chunks;
    while (cursor + 12 <= end) {
        VOXLoader::Chunk chunk;
        chunk.id.assign(reinterpret_cast<const char*>(data.data() + cursor), 4);
        chunk.contentSize = readUint32LE(data, cursor + 4);
        const std::size_t childrenSize = readUint32LE(data, cursor + 8);
        cursor += 12;

        if (cursor + chunk.contentSize > end)
            throw std::runtime_error("THREE.VOXLoader: Chunk is larger than the file");
        chunk.contentOffset = cursor;
        cursor += chunk.contentSize;

        if (childrenSize > 0) {
            if (cursor + childrenSize > end)
                throw std::runtime_error("THREE.VOXLoader: Chunk is larger than the file");
            chunk.children = readChunks(data, cursor, cursor + childrenSize);
            cursor += childrenSize;
        }
        chunks.push_back(std::move(chunk));
    }
    return chunks;
}
}  // namespace

VOXLoader::File VOXLoader::parseChunks(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < 8) throw std::runtime_error("THREE.VOXLoader: Not a VOX file");
    // The signature is the four characters VOX followed by a space.
    if (buffer[0] != 'V' || buffer[1] != 'O' || buffer[2] != 'X' || buffer[3] != ' ')
        throw std::runtime_error("THREE.VOXLoader: Not a VOX file");

    File file;
    file.version = static_cast<int>(readUint32LE(buffer, 4));
    file.chunks = readChunks(buffer, 8, buffer.size());
    return file;
}

VOXLoader::Size VOXLoader::readSize(const std::vector<std::uint8_t>& buffer, const Chunk& chunk) {
    if (chunk.contentSize < 12)
        throw std::runtime_error("THREE.VOXLoader: SIZE chunk is too short");
    Size size;
    size.x = static_cast<int>(readUint32LE(buffer, chunk.contentOffset));
    size.y = static_cast<int>(readUint32LE(buffer, chunk.contentOffset + 4));
    size.z = static_cast<int>(readUint32LE(buffer, chunk.contentOffset + 8));
    return size;
}

std::vector<VOXLoader::Voxel> VOXLoader::readXyzi(const std::vector<std::uint8_t>& buffer,
                                                  const Chunk& chunk) {
    if (chunk.contentSize < 4)
        throw std::runtime_error("THREE.VOXLoader: XYZI chunk is too short");
    const std::uint32_t count = readUint32LE(buffer, chunk.contentOffset);
    if (4 + static_cast<std::size_t>(count) * 4 > chunk.contentSize)
        throw std::runtime_error("THREE.VOXLoader: XYZI chunk is too short");

    std::vector<Voxel> voxels;
    voxels.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::size_t at = chunk.contentOffset + 4 + static_cast<std::size_t>(i) * 4;
        // Three coordinates and a palette index, one byte each.
        Voxel voxel;
        voxel.x = buffer[at + 0];
        voxel.y = buffer[at + 1];
        voxel.z = buffer[at + 2];
        voxel.colorIndex = buffer[at + 3];
        voxels.push_back(voxel);
    }
    return voxels;
}

std::vector<std::array<std::uint8_t, 4>> VOXLoader::readRgba(const std::vector<std::uint8_t>& buffer,
                                                             const Chunk& chunk) {
    const std::size_t count = chunk.contentSize / 4;
    std::vector<std::array<std::uint8_t, 4>> palette;
    palette.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t at = chunk.contentOffset + i * 4;
        palette.push_back({buffer[at + 0], buffer[at + 1], buffer[at + 2], buffer[at + 3]});
    }
    return palette;
}

std::vector<std::uint8_t> VOXLoader::buildOccupancy(const std::vector<Voxel>& voxels,
                                                    const Size& size) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) return {};
    const std::size_t plane = static_cast<std::size_t>(size.x);
    const std::size_t slice = plane * static_cast<std::size_t>(size.y);
    std::vector<std::uint8_t> array(slice * static_cast<std::size_t>(size.z), 0);

    for (const auto& voxel : voxels) {
        // A voxel outside the declared grid would land on another cell, so it is skipped.
        if (voxel.x < 0 || voxel.y < 0 || voxel.z < 0) continue;
        if (voxel.x >= size.x || voxel.y >= size.y || voxel.z >= size.z) continue;
        // Rows run along x, then y, then z, which is the order the texture expects.
        const std::size_t index = static_cast<std::size_t>(voxel.x) +
                                  static_cast<std::size_t>(voxel.y) * plane +
                                  static_cast<std::size_t>(voxel.z) * slice;
        array[index] = 255;
    }
    return array;
}

std::vector<std::uint8_t> VOXLoader::buildColorVolume(const std::vector<Voxel>& voxels,
                                                      const Size& size) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) return {};
    const std::size_t plane = static_cast<std::size_t>(size.x);
    const std::size_t slice = plane * static_cast<std::size_t>(size.y);
    std::vector<std::uint8_t> volume(slice * static_cast<std::size_t>(size.z), 0);

    for (const auto& voxel : voxels) {
        // A voxel outside the grid would land on another cell, so it is skipped.
        if (voxel.x < 0 || voxel.y < 0 || voxel.z < 0) continue;
        if (voxel.x >= size.x || voxel.y >= size.y || voxel.z >= size.z) continue;
        const std::size_t index = static_cast<std::size_t>(voxel.x) +
                                  static_cast<std::size_t>(voxel.y) * plane +
                                  static_cast<std::size_t>(voxel.z) * slice;
        // The mesher reads the palette index back out, so it is stored as it came.
        volume[index] = static_cast<std::uint8_t>(voxel.colorIndex);
    }
    return volume;
}

std::vector<int> VOXLoader::buildMask(const std::vector<std::uint8_t>& volume, const Size& size,
                                      int d, int slice) {
    const int dims[3] = {size.x, size.y, size.z};
    if (d < 0 || d > 2) return {};
    const int axisU = (d + 1) % 3;
    const int axisV = (d + 2) % 3;
    const int dimsD = dims[d];
    const int dimsU = dims[axisU];
    const int dimsV = dims[axisV];
    if (dimsU <= 0 || dimsV <= 0) return {};

    const std::size_t plane = static_cast<std::size_t>(size.x);
    const std::size_t sliceSize = plane * static_cast<std::size_t>(size.y);
    const auto indexAt = [&](int x, int y, int z) -> std::size_t {
        return static_cast<std::size_t>(x) + static_cast<std::size_t>(y) * plane +
               static_cast<std::size_t>(z) * sliceSize;
    };

    std::vector<int> mask(static_cast<std::size_t>(dimsU) * static_cast<std::size_t>(dimsV), 0);
    int n = 0;
    for (int vv = 0; vv < dimsV; ++vv) {
        for (int uu = 0; uu < dimsU; ++uu) {
            int pos[3] = {0, 0, 0};
            pos[d] = slice;
            pos[axisU] = uu;
            pos[axisV] = vv;

            // The cells on each side of the face this slice sits on.
            const int behind = slice > 0
                                   ? volume[indexAt(pos[0], pos[1], pos[2]) -
                                            (d == 2 ? static_cast<int>(sliceSize) : (d == 1 ? size.x : 1))]
                                   : 0;
            const int infront = slice < dimsD ? volume[indexAt(pos[0], pos[1], pos[2])] : 0;

            if (behind > 0 && infront == 0) mask[static_cast<std::size_t>(n)] = behind;
            else if (infront > 0 && behind == 0) mask[static_cast<std::size_t>(n)] = -infront;
            ++n;
        }
    }
    return mask;
}

std::vector<VOXLoader::Quad> VOXLoader::mergeMask(std::vector<int> mask, int dimsU,
                                                  int dimsV) {
    std::vector<Quad> quads;
    if (dimsU <= 0 || dimsV <= 0) return quads;
    const std::size_t width = static_cast<std::size_t>(dimsU);
    if (mask.size() < width * static_cast<std::size_t>(dimsV)) return quads;

    std::size_t n = 0;
    for (int vv = 0; vv < dimsV; ++vv) {
        for (int uu = 0; uu < dimsU;) {
            const int value = mask[n];
            if (value == 0) {
                ++n;
                ++uu;
                continue;
            }

            // The width runs along u while the value matches.
            int w = 1;
            while (uu + w < dimsU && mask[n + static_cast<std::size_t>(w)] == value) ++w;

            // The height grows only while every cell of that whole width matches, so the result
            // stays a rectangle.
            int height = 1;
            bool done = false;
            while (vv + height < dimsV && !done) {
                for (int k = 0; k < w; ++k) {
                    const std::size_t at = n + static_cast<std::size_t>(k) +
                                           static_cast<std::size_t>(height) * width;
                    if (mask[at] != value) {
                        done = true;
                        break;
                    }
                }
                if (!done) ++height;
            }

            Quad quad;
            quad.u = uu;
            quad.v = vv;
            quad.w = w;
            quad.h = height;
            quad.value = value;
            quads.push_back(quad);

            // The merged cells are consumed so the walk does not emit them again; without
            // this the rows after the first would start on already claimed cells.
            for (int row = 0; row < height; ++row)
                for (int k = 0; k < w; ++k)
                    mask[n + static_cast<std::size_t>(k) + static_cast<std::size_t>(row) * width] = 0;

            n += static_cast<std::size_t>(w);
            uu += w;
        }
    }
    return quads;
}

std::array<double, 3> VOXLoader::toThree(const int point[3], const Size& size) {
    // The last two axes swap and the old y is negated, then the model is centred on the origin
    // by subtracting half of each extent.
    return {point[0] - size.x / 2.0, point[2] - size.z / 2.0, -point[1] + size.y / 2.0};
}

VOXLoader::QuadGeometry VOXLoader::emitQuad(const Quad& quad, const int pos[3], const int du[3],
                                            const int dv[3], const Size& size) {
    // The four corners of the rectangle, walking du first and then dv.
    int c0[3] = {pos[0], pos[1], pos[2]};
    int c1[3] = {pos[0] + du[0], pos[1] + du[1], pos[2] + du[2]};
    int c2[3] = {pos[0] + du[0] + dv[0], pos[1] + du[1] + dv[1], pos[2] + du[2] + dv[2]};
    int c3[3] = {pos[0] + dv[0], pos[1] + dv[1], pos[2] + dv[2]};

    const auto p0 = toThree(c0, size);
    const auto p1 = toThree(c1, size);
    const auto p2 = toThree(c2, size);
    const auto p3 = toThree(c3, size);

    QuadGeometry geometry;
    // A positive value faces one way and a negative one the other, which is just a swap of the
    // middle two corners so the triangles wind the other way round.
    const std::array<std::array<double, 3>, 4> order =
        quad.value > 0 ? std::array<std::array<double, 3>, 4>{p0, p1, p2, p3}
                       : std::array<std::array<double, 3>, 4>{p0, p3, p2, p1};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t k = 0; k < 3; ++k) geometry.vertices[i * 3 + k] = order[i][k];

    // Two triangles per quad, sharing the diagonal from the first to the third corner.
    geometry.indices = {0, 1, 2, 0, 2, 3};
    return geometry;
}

VOXLoader::MeshGeometry VOXLoader::buildMesh(const std::vector<Voxel>& voxels, const Size& size,
                                             const std::vector<std::array<std::uint8_t, 4>>& palette) {
    MeshGeometry geometry;
    const std::vector<std::uint8_t> volume = buildColorVolume(voxels, size);
    if (volume.empty()) return geometry;

    const int dims[3] = {size.x, size.y, size.z};
    for (int d = 0; d < 3; ++d) {
        const int axisU = (d + 1) % 3;
        const int axisV = (d + 2) % 3;
        const int dimsU = dims[axisU];
        const int dimsV = dims[axisV];

        // One extra slice so the far end of the volume gets its outward faces.
        for (int slice = 0; slice <= dims[d]; ++slice) {
            const auto mask = buildMask(volume, size, d, slice);
            const auto quads = mergeMask(mask, dimsU, dimsV);

            for (const auto& quad : quads) {
                int pos[3] = {0, 0, 0};
                pos[d] = slice;
                pos[axisU] = quad.u;
                pos[axisV] = quad.v;

                int du[3] = {0, 0, 0};
                int dv[3] = {0, 0, 0};
                du[axisU] = quad.w;
                dv[axisV] = quad.h;

                const auto piece = emitQuad(quad, pos, du, dv, size);
                const int base = static_cast<int>(geometry.positions.size() / 3);
                for (const double component : piece.vertices) geometry.positions.push_back(component);
                for (const int index : piece.indices) geometry.indices.push_back(base + index);

                // The magnitude carries the palette index; the sign only said which way the
                // face looks.
                const std::size_t colorIndex = static_cast<std::size_t>(std::abs(quad.value));
                int rgb[3] = {255, 255, 255};
                if (colorIndex < palette.size()) {
                    rgb[0] = palette[colorIndex][0];
                    rgb[1] = palette[colorIndex][1];
                    rgb[2] = palette[colorIndex][2];
                }
                // The palette is in sRGB, so the four corners share one converted colour.
                const std::vector<double> components{rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0};
                Color color;
                color.fromArray(components);
                ColorManagement::colorSpaceToWorking(color, SRGBColorSpace);
                for (int corner = 0; corner < 4; ++corner) {
                    geometry.colors.push_back(color.r);
                    geometry.colors.push_back(color.g);
                    geometry.colors.push_back(color.b);
                }
            }
        }
    }
    return geometry;
}
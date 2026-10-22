#include "loaders/VOXLoader.h"

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
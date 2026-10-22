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
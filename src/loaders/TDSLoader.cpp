#include "loaders/TDSLoader.h"

#include <cstring>
#include <stdexcept>

namespace {
// Every number in the format is little endian.
std::uint32_t readLE32(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}
std::uint16_t readLE16(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(buffer[at]) |
                                      static_cast<std::uint16_t>(buffer[at + 1] << 8));
}
}  // namespace

TDSLoader::Chunk TDSLoader::readChunk(const std::vector<std::uint8_t>& buffer,
                                      std::size_t offset) {
    Chunk chunk;
    // The header itself is six bytes: a word and a double word.
    if (offset + 6 > buffer.size())
        throw std::runtime_error("THREE.TDSLoader: Invalid chunk header.");

    chunk.offset = offset;
    chunk.id = readLE16(buffer, offset);
    chunk.size = readLE32(buffer, offset + 2);
    // The cursor starts just after those two fields.
    chunk.position = offset + 6;
    // The count covers the header too, so the end is simply that many bytes along.
    chunk.end = offset + chunk.size;
    return chunk;
}

bool TDSLoader::atEnd(const Chunk& chunk) { return chunk.position >= chunk.end; }

std::uint8_t TDSLoader::readByte(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    if (chunk.position + 1 > buffer.size()) return 0;
    return buffer[chunk.position++];
}

std::uint16_t TDSLoader::readWord(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    if (chunk.position + 2 > buffer.size()) return 0;
    const std::uint16_t value = readLE16(buffer, chunk.position);
    chunk.position += 2;
    return value;
}

std::uint32_t TDSLoader::readDWord(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    if (chunk.position + 4 > buffer.size()) return 0;
    const std::uint32_t value = readLE32(buffer, chunk.position);
    chunk.position += 4;
    return value;
}

std::int16_t TDSLoader::readShort(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    return static_cast<std::int16_t>(readWord(buffer, chunk));
}

std::int32_t TDSLoader::readInt(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    return static_cast<std::int32_t>(readDWord(buffer, chunk));
}

float TDSLoader::readFloat(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    if (chunk.position + 4 > buffer.size()) return 0.0f;
    const std::uint32_t bits = readLE32(buffer, chunk.position);
    chunk.position += 4;
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

std::string TDSLoader::readString(const std::vector<std::uint8_t>& buffer, Chunk& chunk) {
    std::string text;
    std::uint8_t c = readByte(buffer, chunk);
    // The string runs to a null, which is consumed as well.
    while (c != 0) {
        text.push_back(static_cast<char>(c));
        c = readByte(buffer, chunk);
    }
    return text;
}

namespace {
// The sub chunk identifiers the readers below understand.
constexpr std::uint16_t kColorF = 0x0010;
constexpr std::uint16_t kColor24 = 0x0011;
constexpr std::uint16_t kLinColor24 = 0x0012;
constexpr std::uint16_t kLinColorF = 0x0013;
constexpr std::uint16_t kIntPercentage = 0x0030;
constexpr std::uint16_t kFloatPercentage = 0x0031;
}  // namespace

TDSLoader::Rgb TDSLoader::readColor(const std::vector<std::uint8_t>& buffer, Chunk& parent) {
    Rgb colour;
    // The colour lives in a sub chunk that begins wherever the parent has reached.
    Chunk sub = readChunk(buffer, parent.position);
    parent.position += sub.size;

    if (sub.id == kColor24 || sub.id == kLinColor24) {
        // Three bytes over the byte range.
        colour.r = readByte(buffer, sub) / 255.0;
        colour.g = readByte(buffer, sub) / 255.0;
        colour.b = readByte(buffer, sub) / 255.0;
        colour.known = true;
    } else if (sub.id == kColorF || sub.id == kLinColorF) {
        // Three floats taken as they stand.
        colour.r = readFloat(buffer, sub);
        colour.g = readFloat(buffer, sub);
        colour.b = readFloat(buffer, sub);
        colour.known = true;
    }
    return colour;
}

double TDSLoader::readPercentage(const std::vector<std::uint8_t>& buffer, Chunk& parent) {
    Chunk sub = readChunk(buffer, parent.position);
    parent.position += sub.size;

    if (sub.id == kIntPercentage) {
        // A short over a hundred.
        return readShort(buffer, sub) / 100.0;
    }
    if (sub.id == kFloatPercentage) {
        return readFloat(buffer, sub);
    }
    // An unknown form reads as nothing.
    return 0.0;
}

std::array<double, 16> TDSLoader::meshMatrix(const std::array<double, 12>& values) {
    // The engine stores a matrix in column major order, so element r of column c sits at
    // r + 4c. The chunk is placed into that layout first and then transposed, which means
    // the value that ends up at row r, column c is the one first placed at row c, column r.
    // Written out, that is the table below.
    std::array<double, 16> matrix{};
    matrix[0] = values[0];
    matrix[1] = values[2];
    matrix[2] = values[1];
    matrix[3] = 0;
    matrix[4] = values[6];
    matrix[5] = values[8];
    matrix[6] = values[7];
    matrix[7] = 0;
    matrix[8] = values[3];
    matrix[9] = values[5];
    matrix[10] = values[4];
    matrix[11] = 0;
    // The last column holds the translation, which the chunk puts at the end of its third row.
    matrix[12] = values[9];
    matrix[13] = values[11];
    matrix[14] = values[10];
    matrix[15] = 1;
    return matrix;
}
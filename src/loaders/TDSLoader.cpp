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

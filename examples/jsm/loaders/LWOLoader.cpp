#include "loaders/LWOLoader.h"

#include <stdexcept>

LWOLoader::ChunkHeader LWOLoader::readChunkHeader(const std::vector<std::uint8_t>& buffer,
                                                  std::size_t offset) {
    ChunkHeader header;
    if (offset + 8 > buffer.size())
        throw std::runtime_error("THREE.LWOLoader: Invalid chunk header.");

    // Four characters name the chunk.
    header.id.assign(reinterpret_cast<const char*>(buffer.data() + offset), 4);
    while (!header.id.empty() && header.id.back() == ' ') header.id.pop_back();

    // The byte count is big endian, as it is throughout the format.
    header.size = (static_cast<std::uint32_t>(buffer[offset + 4]) << 24) |
                  (static_cast<std::uint32_t>(buffer[offset + 5]) << 16) |
                  (static_cast<std::uint32_t>(buffer[offset + 6]) << 8) |
                  static_cast<std::uint32_t>(buffer[offset + 7]);
    header.dataOffset = offset + 8;
    return header;
}

std::string LWOLoader::extractParentUrl(const std::string& url, const std::string& dir) {
    const std::size_t index = url.find(dir);
    // A directory that never appears means the file sits beside the caller.
    if (index == std::string::npos) return "./";
    // Everything before it is the parent.
    return url.substr(0, index);
}

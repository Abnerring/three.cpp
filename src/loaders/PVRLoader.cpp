#include "loaders/PVRLoader.h"

// The third layout starts with the characters PVR followed by a three.
static constexpr std::uint32_t kMagicVersion3 = 0x03525650;
// The second layout ends its header with PVR followed by an exclamation mark, which lands at
// word eleven of a thirteen word header.
static constexpr std::uint32_t kMagicVersion2 = 0x21525650;

std::uint32_t PVRLoader::headerWord(const std::vector<std::uint8_t>& buffer, int index) {
    const std::size_t at = static_cast<std::size_t>(index) * 4;
    if (at + 4 > buffer.size()) return 0;
    // Words are little endian.
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}

int PVRLoader::detectVersion(const std::vector<std::uint8_t>& buffer) {
    // The third version leads with the signature.
    if (headerWord(buffer, 0) == kMagicVersion3) return 3;
    // The second keeps it at the end of the header, so the check is at word eleven.
    if (headerWord(buffer, 11) == kMagicVersion2) return 2;
    return 0;
}

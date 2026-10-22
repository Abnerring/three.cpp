#include "loaders/DDSLoader.h"

#include <stdexcept>

std::uint32_t DDSLoader::headerWord(const std::vector<std::uint8_t>& buffer, int index) {
    const std::size_t at = static_cast<std::size_t>(index) * 4;
    if (at + 4 > buffer.size()) return 0;
    // Words are little endian.
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}

DDSLoader::Header DDSLoader::readHeader(const std::vector<std::uint8_t>& buffer) {
    Header header;
    // The magic comes first, so a buffer too short to hold it is not a DDS file.
    if (buffer.size() < 4 || headerWord(buffer, 0) != kMagic)
        throw std::runtime_error("THREE.DDSLoader.parse: Invalid magic number in DDS header.");

    // The fields sit at fixed words: the size at one, the flags at two, the height at three,
    // the width at four, the mipmap count at seven, and the pixel format details after that.
    header.size = headerWord(buffer, 1);
    header.flags = headerWord(buffer, 2);
    header.height = headerWord(buffer, 3);
    header.width = headerWord(buffer, 4);
    header.mipmapCount = headerWord(buffer, 7);
    header.fourCC = headerWord(buffer, 21);
    header.rgbBitCount = headerWord(buffer, 22);
    header.caps2 = headerWord(buffer, 28);

    // The four characters are packed four in the low byte upwards.
    header.fourCCName.clear();
    for (int i = 0; i < 4; ++i) {
        const std::uint8_t c = static_cast<std::uint8_t>((header.fourCC >> (i * 8)) & 0xff);
        if (c < 32 || c > 126) {
            header.fourCCName.clear();
            break;
        }
        header.fourCCName.push_back(static_cast<char>(c));
    }

    // The payload starts after the declared header size and the four magic bytes.
    header.dataOffset = header.size + 4;
    // A cube map needs the caps2 bit and every one of the six face bits.
    header.isCubemap = (header.caps2 & kCaps2Cubemap) != 0 &&
                       (header.caps2 & kCaps2AllFaces) == kCaps2AllFaces;
    return header;
}

std::vector<DDSLoader::Level> DDSLoader::mipmapChain(const Header& header, int mipmapCount,
                                                     std::size_t firstOffset, int blockBytes,
                                                     int bytesPerPixel) {
    std::vector<Level> levels;
    if (mipmapCount <= 0 || header.width == 0 || header.height == 0) return levels;

    std::uint32_t width = header.width;
    std::uint32_t height = header.height;
    std::size_t offset = firstOffset;
    for (int i = 0; i < mipmapCount; ++i) {
        Level level;
        level.width = width;
        level.height = height;
        level.offset = offset;

        if (blockBytes > 0) {
            // A block covers four by four pixels, so the counts round up.
            const std::size_t blocksWide = (width + 3) / 4;
            const std::size_t blocksHigh = (height + 3) / 4;
            level.length = blocksWide * blocksHigh * static_cast<std::size_t>(blockBytes);
        } else {
            level.length = static_cast<std::size_t>(width) * height *
                           static_cast<std::size_t>(bytesPerPixel);
        }

        levels.push_back(level);
        offset += level.length;
        // Each step halves the size but never reaches zero.
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    return levels;
}
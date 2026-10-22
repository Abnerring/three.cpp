#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the header reading in examples/jsm/loaders/DDSLoader.js at the pinned revision. A
// DDS file is a fixed hundred and twenty four byte header of thirty one words, optionally
// followed by a five word extended header when the format is named DX10, and then the
// compressed payload. The payload needs a GPU format the port does not carry, so the portable
// part is the header itself.
class DDSLoader {
  public:
    // The words the header occupies, and the words the extended header adds.
    static constexpr int kHeaderWords = 31;
    static constexpr int kExtendedHeaderWords = 5;
    static constexpr std::uint32_t kMagic = 0x20534444;
    // The caps2 bit that marks a cube map, and the six face bits that must all be set.
    static constexpr std::uint32_t kCaps2Cubemap = 0x200;
    static constexpr std::uint32_t kCaps2AllFaces = 0xfc00;

    struct Header {
        std::uint32_t size = 0;
        std::uint32_t flags = 0;
        std::uint32_t height = 0;
        std::uint32_t width = 0;
        std::uint32_t mipmapCount = 1;
        // The four characters of the format, packed four in the low byte upwards.
        std::uint32_t fourCC = 0;
        std::uint32_t rgbBitCount = 0;
        std::uint32_t caps2 = 0;
        // Where the payload starts: the declared header size plus the four magic bytes.
        std::size_t dataOffset = 0;
        bool isCubemap = false;
        // The four characters as text, empty when they are not printable.
        std::string fourCCName;
    };

    // Reads one little endian word of the header.
    static std::uint32_t headerWord(const std::vector<std::uint8_t>& buffer, int index);

    // Reads the header. Throws std::runtime_error when the magic is wrong or the buffer is too
    // short, both with the upstream message.
    static Header readHeader(const std::vector<std::uint8_t>& buffer);
};

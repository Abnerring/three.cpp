#include "loaders/PVRLoader.h"

#include <stdexcept>
#include <string>

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

PVRLoader::Header PVRLoader::parseVersion3(const std::vector<std::uint8_t>& buffer) {
    Header header;
    // The fields sit at fixed words: the format at two, the height at six, the width at seven,
    // the face count at ten, the mipmap count at eleven and the metadata length at twelve.
    header.pixelFormat = headerWord(buffer, 2);
    header.height = headerWord(buffer, 6);
    header.width = headerWord(buffer, 7);
    header.numFaces = headerWord(buffer, 10);
    header.numMipmaps = headerWord(buffer, 11);
    header.metadataLength = headerWord(buffer, 12);

    // The four formats the loader understands, two of two bits per pixel and two of four.
    switch (header.pixelFormat) {
        case 0:
            header.bitsPerPixel = 2;
            header.formatName = "RGB_PVRTC_2BPPV1";
            break;
        case 1:
            header.bitsPerPixel = 2;
            header.formatName = "RGBA_PVRTC_2BPPV1";
            break;
        case 2:
            header.bitsPerPixel = 4;
            header.formatName = "RGB_PVRTC_4BPPV1";
            break;
        case 3:
            header.bitsPerPixel = 4;
            header.formatName = "RGBA_PVRTC_4BPPV1";
            break;
        default:
            throw std::runtime_error("THREE.PVRLoader: Unsupported PVR format: " +
                                     std::to_string(header.pixelFormat));
    }

    // The payload starts after the thirteen header words and any metadata.
    header.dataOffset = 13 * 4 + header.metadataLength;
    header.isCubemap = header.numFaces == 6;
    return header;
}

PVRLoader::Header PVRLoader::parseVersion2(const std::vector<std::uint8_t>& buffer) {
    Header header;
    // The second layout orders its fields differently and stores the header length itself.
    const std::uint32_t headerLength = headerWord(buffer, 0);
    header.height = headerWord(buffer, 1);
    header.width = headerWord(buffer, 2);
    const std::uint32_t storedMipmaps = headerWord(buffer, 3);
    const std::uint32_t flags = headerWord(buffer, 4);
    const std::uint32_t alphaMask = headerWord(buffer, 10);
    header.numFaces = headerWord(buffer, 12);

    // The format is the low byte of the flags word.
    const std::uint32_t formatFlags = flags & 0xffu;
    const bool hasAlpha = alphaMask > 0;
    if (formatFlags == 25) {
        header.bitsPerPixel = 4;
        header.formatName = hasAlpha ? "RGBA_PVRTC_4BPPV1" : "RGB_PVRTC_4BPPV1";
    } else if (formatFlags == 24) {
        header.bitsPerPixel = 2;
        header.formatName = hasAlpha ? "RGBA_PVRTC_2BPPV1" : "RGB_PVRTC_2BPPV1";
    } else {
        throw std::runtime_error("THREE.PVRLoader: Unknown PVR format: " +
                                 std::to_string(formatFlags));
    }

    // The stored count is one less than the number of levels, and the payload begins at the
    // header length the file records.
    header.numMipmaps = storedMipmaps + 1;
    header.dataOffset = headerLength;
    header.isCubemap = header.numFaces == 6;
    return header;
}
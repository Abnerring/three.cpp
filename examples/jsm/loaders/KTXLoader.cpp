#include "loaders/KTXLoader.h"

#include <algorithm>

namespace {
// The twelve bytes every KTX 11 file begins with.
const std::uint8_t kIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                      0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};

// Reads a word in the given byte order, from the header block that starts at twelve.
std::uint32_t readWord(const std::vector<std::uint8_t>& buffer, int index, bool littleEndian) {
    const std::size_t at = KTXLoader::kIdentifierBytes + static_cast<std::size_t>(index) * 4;
    if (at + 4 > buffer.size()) return 0;
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        const std::size_t byte = littleEndian ? at + static_cast<std::size_t>(3 - i)
                                              : at + static_cast<std::size_t>(i);
        value = (value << 8) | buffer[byte];
    }
    return value;
}
}  // namespace

KTXLoader::Header KTXLoader::readHeader(const std::vector<std::uint8_t>& buffer,
                                        std::uint32_t facesExpected) {
    Header header;
    // The identifier is twelve fixed bytes.
    if (buffer.size() < kIdentifierBytes) {
        header.error = "texture missing KTX identifier";
        return header;
    }
    for (int i = 0; i < kIdentifierBytes; ++i) {
        if (buffer[static_cast<std::size_t>(i)] != kIdentifier[i]) {
            header.error = "texture missing KTX identifier";
            return header;
        }
    }

    // The first word says the byte order of the rest.
    header.littleEndian = readWord(buffer, 0, true) == 0x04030201;
    const bool little = header.littleEndian;
    header.glType = readWord(buffer, 1, little);
    header.glTypeSize = readWord(buffer, 2, little);
    header.glFormat = readWord(buffer, 3, little);
    header.glInternalFormat = readWord(buffer, 4, little);
    header.glBaseInternalFormat = readWord(buffer, 5, little);
    header.pixelWidth = readWord(buffer, 6, little);
    header.pixelHeight = readWord(buffer, 7, little);
    header.pixelDepth = readWord(buffer, 8, little);
    header.numberOfArrayElements = readWord(buffer, 9, little);
    header.numberOfFaces = readWord(buffer, 10, little);
    header.numberOfMipmapLevels = readWord(buffer, 11, little);
    header.bytesOfKeyValueData = readWord(buffer, 12, little);

    // Only compressed textures, so the type has to be zero.
    if (header.glType != 0) {
        header.error = "only compressed formats currently supported";
        return header;
    }
    // A zero level count means the runtime would generate them, which compressed textures do
    // not allow, so at least one is assumed.
    header.numberOfMipmapLevels = std::max<std::uint32_t>(1, header.numberOfMipmapLevels);

    // Only flat two dimensional textures.
    if (header.pixelHeight == 0 || header.pixelDepth != 0) {
        header.error = "only 2D textures currently supported";
        return header;
    }
    if (header.numberOfArrayElements != 0) {
        header.error = "texture arrays not currently supported";
        return header;
    }
    if (header.numberOfFaces != facesExpected) {
        header.error = "number of faces expected" + std::to_string(facesExpected) +
                       ", but found " + std::to_string(header.numberOfFaces);
        return header;
    }

    header.valid = true;
    return header;
}

std::vector<KTXLoader::Image> KTXLoader::readImages(const std::vector<std::uint8_t>& buffer,
                                                    const Header& header, bool loadMipmaps) {
    std::vector<Image> images;
    if (!header.valid) return images;

    // The image list starts after the header and any key value pairs.
    std::size_t offset = kHeaderLength + header.bytesOfKeyValueData;
    std::uint32_t width = header.pixelWidth;
    std::uint32_t height = header.pixelHeight;
    const std::uint32_t count = loadMipmaps ? header.numberOfMipmapLevels : 1;

    const auto readWordAt = [&buffer, &header](std::size_t at) -> std::uint32_t {
        if (at + 4 > buffer.size()) return 0;
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            const std::size_t byte = header.littleEndian ? at + static_cast<std::size_t>(3 - i)
                                                         : at + static_cast<std::size_t>(i);
            value = (value << 8) | buffer[byte];
        }
        return value;
    };

    for (std::uint32_t level = 0; level < count; ++level) {
        // One size word introduces the level, shared by all of its faces.
        const std::uint32_t imageSize = readWordAt(offset);
        offset += 4;
        for (std::uint32_t face = 0; face < header.numberOfFaces; ++face) {
            Image image;
            image.width = width;
            image.height = height;
            image.imageSize = imageSize;
            // The data begins after the size word.
            image.offset = offset;
            // Padding to the next four byte boundary; an aligned size adds nothing.
            image.padding = 3 - ((static_cast<std::size_t>(imageSize) + 3) % 4);
            images.push_back(image);
            offset += imageSize + image.padding;
        }
        // Each step halves the size but never reaches zero.
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    return images;
}
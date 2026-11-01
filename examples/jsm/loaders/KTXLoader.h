#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container reading in examples/jsm/loaders/KTXLoader.js at the pinned revision. A
// KTX 11 file starts with a twelve byte identifier and thirteen words, the first of which says
// the byte order of the rest. Only compressed two dimensional textures are supported, and
// upstream warns and gives up rather than throwing, so the result carries a reason instead.
class KTXLoader {
  public:
    // The twelve identifier bytes and the words that follow them.
    static constexpr int kIdentifierBytes = 12;
    static constexpr int kHeaderWords = 13;
    static constexpr std::size_t kHeaderLength = kIdentifierBytes + kHeaderWords * 4;

    struct Header {
        // False when the file was rejected, with the reason in error.
        bool valid = false;
        std::string error;
        bool littleEndian = true;
        std::uint32_t glType = 0;
        std::uint32_t glTypeSize = 0;
        std::uint32_t glFormat = 0;
        // The value passed to the compressed upload call, which names the format.
        std::uint32_t glInternalFormat = 0;
        std::uint32_t glBaseInternalFormat = 0;
        std::uint32_t pixelWidth = 0;
        std::uint32_t pixelHeight = 0;
        std::uint32_t pixelDepth = 0;
        std::uint32_t numberOfArrayElements = 0;
        std::uint32_t numberOfFaces = 0;
        std::uint32_t numberOfMipmapLevels = 0;
        std::uint32_t bytesOfKeyValueData = 0;
    };

    // Reads the header and applies the same checks upstream does. The face count the caller
    // expects is one for a flat texture and six for a cube map.
    static Header readHeader(const std::vector<std::uint8_t>& buffer,
                             std::uint32_t facesExpected = 1);

    // One image in the file. The offset is where its bytes begin, which is after the size word
    // that introduces its level rather than at the start of that word.
    struct Image {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t imageSize = 0;
        std::size_t offset = 0;
        // Bytes of alignment added after the image so the next one starts on a four byte
        // boundary. A size already aligned adds nothing.
        std::size_t padding = 0;
    };

    // Walks the image list. Each level begins with the size shared by its faces, and each face
    // is followed by padding to the next four byte boundary. Each step halves the size but
    // never reaches zero. With loadMipmaps false only the first level is reported.
    static std::vector<Image> readImages(const std::vector<std::uint8_t>& buffer,
                                         const Header& header, bool loadMipmaps = true);
};

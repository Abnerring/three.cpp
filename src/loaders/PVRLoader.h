#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container detection in examples/jsm/loaders/PVRLoader.js at the pinned revision.
// A PVR file is a compressed texture container with two on disk layouts: the third version
// leads with its signature and the second puts it at the end, so a reader has to look in both
// places. The compressed payload needs a GPU format the port does not carry, so only the
// detection and the header fields are portable.
class PVRLoader {
  public:
    // The number of words the common header occupies.
    static constexpr int kHeaderWords = 13;

    // Which layout a buffer holds: three, two, or zero when neither signature is found.
    static int detectVersion(const std::vector<std::uint8_t>& buffer);

    // Reads one little endian word of the header.
    static std::uint32_t headerWord(const std::vector<std::uint8_t>& buffer, int index);

    // The fields of a third version header that matter for a texture.
    struct Header {
        std::uint32_t pixelFormat = 0;
        // Bits per pixel, which follows from the pixel format.
        int bitsPerPixel = 0;
        // The name of the format, or empty when the pixel format is not one the loader knows.
        std::string formatName;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t numFaces = 0;
        std::uint32_t numMipmaps = 0;
        std::uint32_t metadataLength = 0;
        // Where the compressed payload begins: the thirteen header words plus the metadata.
        std::size_t dataOffset = 0;
        bool isCubemap = false;
    };

    // Reads a third version header. Throws std::runtime_error for a pixel format it cannot
    // describe, naming the format the way upstream does.
    static Header parseVersion3(const std::vector<std::uint8_t>& buffer);
};

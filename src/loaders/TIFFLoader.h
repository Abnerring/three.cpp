#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the portable part of examples/jsm/loaders/TIFFLoader.js at the pinned revision. The
// upstream loader decodes pixels with the bundled UTIF library; the port has no such decoder,
// so it reads the header itself, which is enough to report the image geometry, and refuses the
// pixel decoding explicitly rather than returning empty data.
class TIFFLoader {
  public:
    // The fields of the first image file directory that matter for a texture.
    struct Header {
        bool littleEndian = true;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint16_t bitsPerSample = 1;
        // One means the samples are stored as they are; anything else is a compression the
        // port cannot undo.
        std::uint16_t compression = 1;
        // How many directories the file lists.
        std::uint32_t directoryCount = 0;
    };

    // Reads the byte order mark, the magic number and the first directory. Throws
    // std::runtime_error when the magic is wrong or a directory runs past the buffer.
    static Header readHeader(const std::vector<std::uint8_t>& buffer);

    // Always throws: the pixel decoding needs the UTIF library, which the port does not carry.
    static void requireDecoder();
};

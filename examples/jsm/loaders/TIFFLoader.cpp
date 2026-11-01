#include "loaders/TIFFLoader.h"

#include <stdexcept>

namespace {
// Reads an integer of the given width in the file's byte order.
std::uint32_t readValue(const std::vector<std::uint8_t>& data, std::size_t at, int width,
                        bool littleEndian) {
    std::uint32_t value = 0;
    for (int i = 0; i < width; ++i) {
        const std::size_t index = littleEndian ? at + static_cast<std::size_t>(width - 1 - i)
                                               : at + static_cast<std::size_t>(i);
        value = (value << 8) | data[index];
    }
    return value;
}
}  // namespace

TIFFLoader::Header TIFFLoader::readHeader(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < 8) throw std::runtime_error("THREE.TIFFLoader: Not a TIFF file");

    Header header;
    // The first two bytes give the byte order: two Is or two Ms.
    if (buffer[0] == 0x49 && buffer[1] == 0x49) header.littleEndian = true;
    else if (buffer[0] == 0x4d && buffer[1] == 0x4d) header.littleEndian = false;
    else throw std::runtime_error("THREE.TIFFLoader: Not a TIFF file");

    // The magic number is forty two in that order.
    if (readValue(buffer, 2, 2, header.littleEndian) != 42)
        throw std::runtime_error("THREE.TIFFLoader: Not a TIFF file");

    const std::size_t directoryAt = readValue(buffer, 4, 4, header.littleEndian);
    if (directoryAt + 2 > buffer.size())
        throw std::runtime_error("THREE.TIFFLoader: Directory runs past the file");

    const std::uint32_t entries = readValue(buffer, directoryAt, 2, header.littleEndian);
    header.directoryCount = entries;
    for (std::uint32_t i = 0; i < entries; ++i) {
        // Each entry is a tag, a type, a count and a value, twelve bytes in all.
        const std::size_t at = directoryAt + 2 + static_cast<std::size_t>(i) * 12;
        if (at + 12 > buffer.size())
            throw std::runtime_error("THREE.TIFFLoader: Directory runs past the file");

        const std::uint32_t tag = readValue(buffer, at, 2, header.littleEndian);
        const std::uint32_t type = readValue(buffer, at + 2, 2, header.littleEndian);
        // A short is two bytes and a long four, both of which fit in the value field.
        const int width = type == 3 ? 2 : (type == 4 ? 4 : 0);
        if (width == 0) continue;
        const std::uint32_t value = readValue(buffer, at + 8, width, header.littleEndian);

        // The three tags a texture needs, plus the compression it cannot undo.
        if (tag == 256) header.width = value;
        else if (tag == 257) header.height = value;
        else if (tag == 258) header.bitsPerSample = static_cast<std::uint16_t>(value);
        else if (tag == 259) header.compression = static_cast<std::uint16_t>(value);
    }
    return header;
}

void TIFFLoader::requireDecoder() {
    throw std::runtime_error("THREE.TIFFLoader: Decoding needs the UTIF library, which is not ported");
}

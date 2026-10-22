#include "loaders/TGALoader.h"

#include <stdexcept>

namespace {
int readUint16LE(const std::vector<std::uint8_t>& data, std::size_t at) {
    return int(data[at]) | (int(data[at + 1]) << 8);
}
}  // namespace

bool TGALoader::isRle(int imageType) {
    return imageType == kTypeRleIndexed || imageType == kTypeRleRgb || imageType == kTypeRleGrey;
}

void TGALoader::checkHeader(const Header& header) {
    switch (header.imageType) {
        case kTypeIndexed:
        case kTypeRleIndexed:
            // An indexed image needs a colour map, of at most two hundred and fifty six
            // entries, each twenty four bits wide.
            if (header.colormapLength > 256 || header.colormapSize != 24 || header.colormapType != 1)
                throw std::runtime_error("THREE.TGALoader: Invalid type colormap data for indexed type.");
            break;

        case kTypeRgb:
        case kTypeGrey:
        case kTypeRleRgb:
        case kTypeRleGrey:
            // The direct colour types must not carry a colour map.
            if (header.colormapType)
                throw std::runtime_error("THREE.TGALoader: Invalid type colormap data for colormap type.");
            break;

        case kTypeNoData:
            throw std::runtime_error("THREE.TGALoader: No data.");

        default:
            // The message names the type that was found.
            throw std::runtime_error("THREE.TGALoader: Invalid type " +
                                     std::to_string(header.imageType));
    }

    // A file without a positive size has nothing to draw.
    if (header.width <= 0 || header.height <= 0)
        throw std::runtime_error("THREE.TGALoader: Invalid image size.");

    // The depth has to be one of the four the decoder handles; fifteen is not among them.
    if (header.pixelSize != 8 && header.pixelSize != 16 && header.pixelSize != 24 &&
        header.pixelSize != 32)
        throw std::runtime_error("THREE.TGALoader: Invalid pixel size " +
                                 std::to_string(header.pixelSize));
}

TGALoader::Header TGALoader::readHeader(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < 18)
        throw std::runtime_error("THREE.TGALoader: Invalid header length.");

    // The layout is fixed: three single byte fields, three colour map fields, two origins, the
    // size, the pixel depth and the descriptor.
    Header header;
    header.idLength = buffer[0];
    header.colormapType = buffer[1];
    header.imageType = buffer[2];
    header.colormapFirst = readUint16LE(buffer, 3);
    header.colormapLength = readUint16LE(buffer, 5);
    header.colormapSize = buffer[7];
    header.xOrigin = readUint16LE(buffer, 8);
    header.yOrigin = readUint16LE(buffer, 10);
    header.width = readUint16LE(buffer, 12);
    header.height = readUint16LE(buffer, 14);
    header.pixelSize = buffer[16];
    header.descriptor = buffer[17];
    return header;
}

std::vector<std::uint8_t> TGALoader::decodeRle(const std::vector<std::uint8_t>& data,
                                               std::size_t offset, std::size_t pixelTotal,
                                               int pixelSizeBytes) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pixelSizeBytes), 0);
    std::vector<std::uint8_t> out(pixelTotal, 0);
    std::size_t shift = 0;

    while (shift < pixelTotal) {
        if (offset >= data.size())
            throw std::runtime_error("THREE.TGALoader: Unexpected end of pixel data");
        const std::uint8_t control = data[offset++];
        // The low seven bits count one less than the number of pixels.
        std::size_t count = static_cast<std::size_t>(control & 0x7f) + 1;

        if (control & 0x80) {
            // A repeat packet: one pixel, copied count times.
            for (int i = 0; i < pixelSizeBytes; ++i) {
                if (offset >= data.size())
                    throw std::runtime_error("THREE.TGALoader: Unexpected end of pixel data");
                pixels[static_cast<std::size_t>(i)] = data[offset++];
            }
            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t at = shift + i * static_cast<std::size_t>(pixelSizeBytes);
                for (int k = 0; k < pixelSizeBytes && at + static_cast<std::size_t>(k) < pixelTotal; ++k)
                    out[at + static_cast<std::size_t>(k)] = pixels[static_cast<std::size_t>(k)];
            }
            shift += static_cast<std::size_t>(pixelSizeBytes) * count;
        } else {
            // A raw packet: count pixels follow as they are.
            count *= static_cast<std::size_t>(pixelSizeBytes);
            for (std::size_t i = 0; i < count; ++i) {
                if (offset >= data.size())
                    throw std::runtime_error("THREE.TGALoader: Unexpected end of pixel data");
                const std::uint8_t value = data[offset++];
                if (shift + i < pixelTotal) out[shift + i] = value;
            }
            shift += count;
        }
    }
    return out;
}
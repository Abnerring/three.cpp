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
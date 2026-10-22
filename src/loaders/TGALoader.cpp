#include "loaders/TGALoader.h"

#include <stdexcept>

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

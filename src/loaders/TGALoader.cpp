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

int TGALoader::originOf(int descriptor) {
    // Bits four and five of the descriptor pick one of four origins.
    return (descriptor & kOriginMask) >> kOriginShift;
}

std::vector<std::uint8_t> TGALoader::toRGBA(const std::vector<std::uint8_t>& pixels,
                                            int pixelSize) {
    const std::size_t bytes = static_cast<std::size_t>(pixelSize / 8);
    if (bytes < 3) return {};
    const std::size_t count = pixels.size() / bytes;
    std::vector<std::uint8_t> out(count * 4, 0);

    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t at = i * bytes;
        // The file stores blue, green, red then optionally alpha; the working order is red,
        // green, blue, alpha.
        out[i * 4 + 0] = pixels[at + 2];
        out[i * 4 + 1] = pixels[at + 1];
        out[i * 4 + 2] = pixels[at + 0];
        out[i * 4 + 3] = bytes == 4 ? pixels[at + 3] : std::uint8_t(255);
    }
    return out;
}

std::vector<std::uint8_t> TGALoader::toRGBAFromGrey8(const std::vector<std::uint8_t>& pixels) {
    std::vector<std::uint8_t> out(pixels.size() * 4, 0);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        // A grey sample feeds all three channels.
        out[i * 4 + 0] = pixels[i];
        out[i * 4 + 1] = pixels[i];
        out[i * 4 + 2] = pixels[i];
        out[i * 4 + 3] = 255;
    }
    return out;
}

std::vector<std::uint8_t> TGALoader::toRGBAFromGrey16(const std::vector<std::uint8_t>& pixels) {
    const std::size_t count = pixels.size() / 2;
    std::vector<std::uint8_t> out(count * 4, 0);
    for (std::size_t i = 0; i < count; ++i) {
        // Two little endian bytes per sample; the high byte carries the luminance.
        const std::uint8_t value = pixels[i * 2 + 1];
        out[i * 4 + 0] = value;
        out[i * 4 + 1] = value;
        out[i * 4 + 2] = value;
        out[i * 4 + 3] = 255;
    }
    return out;
}

std::vector<std::uint8_t> TGALoader::toRGBAFrom16(const std::vector<std::uint8_t>& pixels) {
    const std::size_t count = pixels.size() / 2;
    std::vector<std::uint8_t> out(count * 4, 0);
    for (std::size_t i = 0; i < count; ++i) {
        const int packed = int(pixels[i * 2]) | (int(pixels[i * 2 + 1]) << 8);
        // Five bits per channel, expanded by shifting left three so one becomes two hundred
        // and forty eight rather than one.
        out[i * 4 + 0] = static_cast<std::uint8_t>(((packed >> 10) & 0x1f) << 3);
        out[i * 4 + 1] = static_cast<std::uint8_t>(((packed >> 5) & 0x1f) << 3);
        out[i * 4 + 2] = static_cast<std::uint8_t>((packed & 0x1f) << 3);
        // The top bit is a one bit alpha in some writers, but the port treats every sixteen
        // bit pixel as opaque, which is what the common files mean.
        out[i * 4 + 3] = 255;
    }
    return out;
}

void TGALoader::flipToOrigin(std::vector<std::uint8_t>& pixels, int width, int height, int origin) {
    const bool flipRows = origin >= kOriginTopLeft;
    const bool flipColumns = (origin & 1) != 0;
    if (!flipRows && !flipColumns) return;

    const std::size_t w = static_cast<std::size_t>(width);
    const std::size_t h = static_cast<std::size_t>(height);
    if (pixels.size() < w * h * 4) return;

    std::vector<std::uint8_t> copy = pixels;
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            // Both flips are mirrorings around the middle of their axis.
            const std::size_t sourceY = flipRows ? h - 1 - y : y;
            const std::size_t sourceX = flipColumns ? w - 1 - x : x;
            const std::size_t from = (sourceY * w + sourceX) * 4;
            const std::size_t to = (y * w + x) * 4;
            for (int k = 0; k < 4; ++k) pixels[to + k] = copy[from + k];
        }
    }
}

std::vector<std::uint8_t> TGALoader::toRGBAFromIndexed(const std::vector<std::uint8_t>& pixels,
                                                       const std::vector<std::uint8_t>& palette) {
    const std::size_t entries = palette.size() / 3;
    std::vector<std::uint8_t> out(pixels.size() * 4, 0);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        const std::size_t index = pixels[i];
        if (index < entries) {
            const std::size_t at = index * 3;
            // A palette entry has the same blue green red order as a twenty four bit pixel.
            out[i * 4 + 0] = palette[at + 2];
            out[i * 4 + 1] = palette[at + 1];
            out[i * 4 + 2] = palette[at + 0];
        }
        // An index past the end stays black rather than reading past the palette.
        out[i * 4 + 3] = 255;
    }
    return out;
}
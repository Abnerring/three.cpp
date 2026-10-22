#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the header validation in examples/jsm/loaders/TGALoader.js at the pinned revision.
// TGA begins with a fixed eighteen byte header describing a colour map and the image; the
// loader rejects combinations it cannot decode. Reading the pixels is the layer above.
class TGALoader {
  public:
    // The image types the format defines.
    static constexpr int kTypeNoData = 0;
    static constexpr int kTypeIndexed = 1;
    static constexpr int kTypeRgb = 2;
    static constexpr int kTypeGrey = 3;
    static constexpr int kTypeRleIndexed = 9;
    static constexpr int kTypeRleRgb = 10;
    static constexpr int kTypeRleGrey = 11;

    struct Header {
        int idLength = 0;
        int colormapType = 0;
        int imageType = 0;
        int colormapFirst = 0;
        int colormapLength = 0;
        int colormapSize = 0;
        // The descriptor packs the alpha channel bit count and the origin flags.
        int width = 0;
        int height = 0;
        int pixelSize = 0;
        int xOrigin = 0;
        int yOrigin = 0;
        int descriptor = 0;
    };

    // Reads the fixed eighteen byte header: a byte of identifier length, a colour map flag and
    // an image type, then the colour map specification, the two origin fields, the size, the
    // pixel depth and the descriptor. The multi byte fields are little endian.
    static Header readHeader(const std::vector<std::uint8_t>& buffer);

    // Applies the upstream checks to a header: an indexed image needs a twenty four bit colour
    // map of at most two hundred and fifty six entries, the direct colour types must not
    // declare one, a no data image is rejected, an unknown type is named in the error, and a
    // non positive size is rejected. Throws std::runtime_error with the upstream message.
    static void checkHeader(const Header& header);

    // Whether a type is one of the run length encoded ones.
    static bool isRle(int imageType);
};

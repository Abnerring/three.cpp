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

    // Expands run length encoded pixels into a flat buffer of pixelTotal bytes. A control byte
    // whose high bit is set repeats one pixel count times, otherwise count pixels follow
    // literally. Throws when the data ends early.
    static std::vector<std::uint8_t> decodeRle(const std::vector<std::uint8_t>& data,
                                               std::size_t offset, std::size_t pixelTotal,
                                               int pixelSizeBytes);

    // The origin flags the descriptor carries in bits four and five: zero is bottom left, one
    // bottom right, two top left and three top right. The two upper origins need the rows
    // flipped, and the two right origins need the columns flipped.
    static constexpr int kOriginMask = 0x30;
    static constexpr int kOriginShift = 4;
    static constexpr int kOriginBottomLeft = 0;
    static constexpr int kOriginBottomRight = 1;
    static constexpr int kOriginTopLeft = 2;
    static constexpr int kOriginTopRight = 3;
    static int originOf(int descriptor);

    // Expands 24 bit BGR or 32 bit BGRA pixels into RGBA, swapping the channels so the result
    // is in the order the rest of the port uses. The 32 bit form keeps its alpha byte.
    static std::vector<std::uint8_t> toRGBA(const std::vector<std::uint8_t>& pixels, int pixelSize);

    // Eight bit grey: one byte per pixel, copied into all three channels with full alpha.
    static std::vector<std::uint8_t> toRGBAFromGrey8(const std::vector<std::uint8_t>& pixels);

    // Sixteen bit grey: two little endian bytes per pixel, the luminance taken from the high
    // byte and copied into all three channels.
    static std::vector<std::uint8_t> toRGBAFromGrey16(const std::vector<std::uint8_t>& pixels);

    // Sixteen bit colour: five bits per channel packed little endian, expanded to eight bits
    // by shifting left three so the full range is covered.
    static std::vector<std::uint8_t> toRGBAFrom16(const std::vector<std::uint8_t>& pixels);

    // Reorders an RGBA image so the first row and column are the top left one. The two upper
    // origins need the rows reversed and the two right origins the columns.
    static void flipToOrigin(std::vector<std::uint8_t>& pixels, int width, int height, int origin);

    // Whether a type is one of the run length encoded ones.
    static bool isRle(int imageType);
};

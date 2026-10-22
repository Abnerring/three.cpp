#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Port of the LZF decompression and header parsing in examples/jsm/loaders/PCDLoader.js at
// the pinned revision. PCD is a point cloud format: an ascii header, then ascii, binary or
// LZF compressed binary point data. This slice covers the decompressor and the header; the
// point decoding is the layer above.
class PCDLoader {
  public:
    struct Header {
        // One entry per field of the points: x, y, z, rgb, an intensity and so on.
        std::vector<std::string> fields;
        std::vector<int> size;
        std::vector<std::string> type;
        std::vector<int> count;
        int width = 0;
        int height = 0;
        int points = 0;
        // "ascii", "binary" or "binary_compressed".
        std::string data;
        // How many bytes of the buffer the header text occupied.
        std::size_t headerLength = 0;
    };

    // Decompresses LZF data into exactly outLength bytes. Throws std::runtime_error with the
    // upstream message for each malformed case.
    static std::vector<std::uint8_t> decompressLZF(const std::vector<std::uint8_t>& inData,
                                                   std::size_t outLength);

    // Reads the ascii header up to and including the data line. Throws when the header ends
    // before a data type is declared.
    static Header parseHeader(const std::vector<std::uint8_t>& buffer);

    // The per field lookup a point parser needs: for ascii an offset is a token index and
    // grows by the field count, for the binary forms it is a byte offset and grows by size
    // times count. Returned rather than stored on the header, so the header value type keeps
    // its plain layout.
    struct FieldLayout {
        std::map<std::string, int> offset;
        // Bytes per point; only meaningful for the binary forms.
        int rowSize = 0;
    };
    static FieldLayout computeOffsets(const Header& header);

    // The streams a point cloud decodes into.
    struct Points {
        std::vector<double> positions;
        std::vector<double> normals;
        // Three working space components per point, already converted out of sRGB.
        std::vector<double> colors;
        std::vector<double> intensity;
        std::vector<double> label;
    };

    // Splits a packed PCD rgb value into three channel bytes. The order is red in the
    // highest byte, then green, then blue.
    static void unpackRGB(unsigned int packed, int rgb[3]);

    // Reads ascii point rows. Each row is whitespace separated and the layout says which
    // token belongs to which field, so a missing field simply contributes nothing.
    static Points parseAsciiPoints(const std::string& text, const FieldLayout& layout);

    // Reads binary point rows. The header supplies each field's width and type and the
    // layout supplies its byte offset, so a row is walked field by field. Floats are read as
    // floats and the integer types are widened to double, then the same streams are filled
    // as for the ascii form.
    static Points parseBinaryPoints(const std::vector<std::uint8_t>& data, std::size_t start,
                                    int pointCount, const Header& header,
                                    const FieldLayout& layout, bool littleEndian);
};

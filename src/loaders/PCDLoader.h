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
};

#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Port of the header half of examples/jsm/loaders/NRRDLoader.js at the pinned revision. NRRD
// is a medical imaging container: an ascii header of key and value lines, a blank line, then
// the voxel data. This slice reads the header and derives the direction vectors; decoding the
// body is the layer above.
class NRRDLoader {
  public:
    struct Header {
        bool isNrrd = false;
        // Free form fields, including the ones with typed handling below.
        std::map<std::string, std::string> fields;
        std::string encoding;
        int dimension = 0;
        std::vector<double> sizes;
        std::vector<double> spacings;
        // Direction vectors, one per axis. Empty until resolved.
        std::vector<std::array<double, 3>> vectors;
    };

    // Parses the key and value lines. Throws std::runtime_error when the magic is missing or
    // the encoding is one of the compressed bzip forms, which the loader does not support.
    static Header parseHeader(const std::string& text);

    // Fills the direction vectors: the identity when the file gave no space directions, with
    // each row scaled by the matching spacing when spacings were declared.
    static void resolveVectors(Header& header);

    // Reads the body when it is written as text. Numbers are separated by whitespace, tab
    // through carriage return and the space character, and the count is the product of the
    // sizes. A floating point type parses fractions while an integer type truncates them,
    // which is the difference between parseFloat and parseInt upstream. The hex encoding
    // reads base sixteen.
    static std::vector<double> parseTextData(const std::string& text,
                                             const std::vector<double>& sizes,
                                             bool floating, bool hex);

    // Bytes per voxel for a NRRD type name, or zero when the name is unknown.
    static int typeWidth(const std::string& type);

    // Reads count voxels of the given type from the binary body. Upstream hard codes little
    // endian and never consults the endian field, so the same is done here rather than
    // silently differing from it. An unknown type throws.
    static std::vector<double> parseBinaryData(const std::vector<std::uint8_t>& data,
                                               std::size_t start, std::size_t count,
                                               const std::string& type);
};

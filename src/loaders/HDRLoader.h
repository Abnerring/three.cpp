#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of examples/jsm/loaders/HDRLoader.js at the pinned revision: the RGBE header parser,
// the run length decoder and the float conversion. What is not ported is the browser data
// texture loader base, so parse() takes the bytes and returns the decoded pixels.
class HDRLoader {
  public:
    struct Header {
        bool hasProgramType = false;
        bool hasFormat = false;
        bool hasDimensions = false;
        std::string programtype = "RGBE";
        std::string format;
        std::string comments;
        double gamma = 1.0;
        double exposure = 1.0;
        int width = 0;
        int height = 0;
    };

    struct Result {
        Header header;
        // RGBA floats, four per pixel, from top to bottom as the file stores them.
        std::vector<float> data;
    };

    // Throws std::runtime_error with the upstream messages when the header or the scanlines
    // do not parse.
    Result parse(const std::vector<std::uint8_t>& buffer) const;

    // Decodes one RGBE quadruple into RGBA floats. Exposed because it is the whole colour
    // maths of the format.
    static void rgbeToFloat(const std::uint8_t* source, std::size_t offset, float* dest,
                            std::size_t destOffset);
};

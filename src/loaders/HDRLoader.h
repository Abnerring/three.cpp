#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of examples/jsm/loaders/HDRLoader.js at the pinned revision: the RGBE header parser,
// the run length decoder and the float conversion. What is not ported is the browser data
// texture loader base, so parse() takes the bytes and returns the decoded pixels.
class HDRLoader {
  public:
    // HalfFloatType upstream, which is what the HDR cube loader defaults to.
    static constexpr double kHalfFloatType = 1016;
    static constexpr double kFloatType = 1015;
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

    // DataTextureLoader exposes the target texture type through this setter upstream; the
    // parser itself does not branch on it, so it is stored and reported back.
    HDRLoader& setDataType(double value) {
        _type = value;
        return *this;
    }
    double dataType() const { return _type; }

    // Throws std::runtime_error with the upstream messages when the header or the scanlines
    // do not parse.
    Result parse(const std::vector<std::uint8_t>& buffer) const;

    // Decodes one RGBE quadruple into RGBA floats. Exposed because it is the whole colour
    // maths of the format.
    static void rgbeToFloat(const std::uint8_t* source, std::size_t offset, float* dest,
                            std::size_t destOffset);

  private:
    double _type = kHalfFloatType;
};

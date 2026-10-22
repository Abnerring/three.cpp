#include "loaders/HDRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
void pushText(std::vector<std::uint8_t>& out, const std::string& text) {
    out.insert(out.end(), text.begin(), text.end());
}

// Two by two, which is below the run length threshold, so the pixels are stored flat.
std::vector<std::uint8_t> makeFlat() {
    std::vector<std::uint8_t> data;
    pushText(data, "#?RADIANCE\n");
    pushText(data, "# a comment\n");
    pushText(data, "FORMAT=32-bit_rle_rgbe\n");
    pushText(data, "-Y 2 +X 2\n");
    // Four RGBE quadruples.
    const std::uint8_t pixels[16] = {128, 64, 32, 128, 0, 0, 0, 0, 255, 255, 255, 128, 1, 2, 3, 129};
    data.insert(data.end(), pixels, pixels + 16);
    return data;
}
}  // namespace

TEST_CASE("HDRLoader decodes the RGBE scale exactly") {
    float out[4] = {0, 0, 0, 0};
    const std::uint8_t pixel[4] = {128, 64, 32, 128};
    HDRLoader::rgbeToFloat(pixel, 0, out, 0);
    // The exponent of 128 gives a scale of one over two hundred and fifty five.
    REQUIRE(out[0] == Approx(128.0 / 255.0).epsilon(1e-6));
    REQUIRE(out[1] == Approx(64.0 / 255.0).epsilon(1e-6));
    REQUIRE(out[2] == Approx(32.0 / 255.0).epsilon(1e-6));
    REQUIRE(out[3] == 1.0f);

    // A black pixel stays black, and a zero exponent collapses everything.
    const std::uint8_t black[4] = {0, 0, 0, 0};
    HDRLoader::rgbeToFloat(black, 0, out, 0);
    REQUIRE(out[0] == 0.0f);
    REQUIRE(out[3] == 1.0f);
}

TEST_CASE("HDRLoader reads the header fields") {
    HDRLoader loader;
    const auto result = loader.parse(makeFlat());
    REQUIRE(result.header.hasProgramType);
    REQUIRE(result.header.programtype == "RADIANCE");
    REQUIRE(result.header.hasFormat);
    REQUIRE(result.header.format == "32-bit_rle_rgbe");
    REQUIRE(result.header.hasDimensions);
    REQUIRE(result.header.width == 2);
    REQUIRE(result.header.height == 2);
    REQUIRE(result.header.comments == "# a comment\n");
    REQUIRE(result.header.gamma == 1.0);
    REQUIRE(result.header.exposure == 1.0);
}

TEST_CASE("HDRLoader decodes a flat pixel block") {
    HDRLoader loader;
    const auto result = loader.parse(makeFlat());
    REQUIRE(result.data.size() == 16);
    // The first pixel is the one checked above.
    REQUIRE(result.data[0] == Approx(128.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[1] == Approx(64.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[2] == Approx(32.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[3] == 1.0f);
    // The second pixel is black.
    REQUIRE(result.data[4] == 0.0f);
    REQUIRE(result.data[7] == 1.0f);
}

TEST_CASE("HDRLoader decodes a run length encoded scanline") {
    // Width eight takes the encoded path, with literal runs of eight in each channel.
    std::vector<std::uint8_t> data;
    pushText(data, "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n-Y 1 +X 8\n");
    data.push_back(2);
    data.push_back(2);
    data.push_back(0);
    data.push_back(8);
    for (int channel = 0; channel < 4; ++channel) {
        data.push_back(8);  // a literal run of eight values, since the count is not above 128
        for (int i = 0; i < 8; ++i)
            data.push_back(channel == 3 ? 128 : std::uint8_t(channel == 0 ? 200 : 100));
    }

    HDRLoader loader;
    const auto result = loader.parse(data);
    REQUIRE(result.header.width == 8);
    REQUIRE(result.header.height == 1);
    REQUIRE(result.data.size() == 32);
    // Red channel 200 with exponent 128 scales to 200 over 255.
    REQUIRE(result.data[0] == Approx(200.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[1] == Approx(100.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[2] == Approx(100.0 / 255.0).epsilon(1e-6));
}

TEST_CASE("HDRLoader reports the upstream header errors") {
    HDRLoader loader;
    std::vector<std::uint8_t> empty;
    REQUIRE_THROWS_WITH(loader.parse(empty), "THREE.HDRLoader: bad initial token");

    std::vector<std::uint8_t> noFormat;
    pushText(noFormat, "#?RADIANCE\n-Y 2 +X 2\n");
    REQUIRE_THROWS_WITH(loader.parse(noFormat), "THREE.HDRLoader: missing format specifier");

    std::vector<std::uint8_t> noSize;
    pushText(noSize, "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n");
    REQUIRE_THROWS_WITH(loader.parse(noSize), "THREE.HDRLoader: missing image size specifier");
}

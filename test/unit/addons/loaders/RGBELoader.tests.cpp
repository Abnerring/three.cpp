#include "loaders/HDRLoader.h"
#include "loaders/RGBELoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> makeFlat() {
    const std::string header = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n-Y 1 +X 2\n";
    std::vector<std::uint8_t> data(header.begin(), header.end());
    const std::uint8_t pixels[8] = {128, 64, 32, 128, 0, 0, 0, 0};
    data.insert(data.end(), pixels, pixels + 8);
    return data;
}
}  // namespace

TEST_CASE("RGBELoader is HDRLoader with a deprecation notice") {
    REQUIRE(std::string(RGBELoader::deprecatedMessage()) ==
            "RGBELoader has been deprecated. Please use HDRLoader instead.");

    // Parsing is inherited unchanged, so the decoded pixel matches the HDR loader exactly.
    RGBELoader loader;
    const auto result = loader.parse(makeFlat());
    REQUIRE(result.header.width == 2);
    REQUIRE(result.header.height == 1);
    REQUIRE(result.data.size() == 8);
    REQUIRE(result.data[0] == Approx(128.0 / 255.0).epsilon(1e-6));
    REQUIRE(result.data[3] == 1.0f);
    REQUIRE(result.data[4] == 0.0f);
}

TEST_CASE("HDRLoader reports the data type it was given") {
    HDRLoader loader;
    // Half float is the default the cube loader uses.
    REQUIRE(loader.dataType() == HDRLoader::kHalfFloatType);
    loader.setDataType(HDRLoader::kFloatType);
    REQUIRE(loader.dataType() == HDRLoader::kFloatType);
    // The setter is chainable upstream, so it returns the loader.
    REQUIRE(&loader.setDataType(HDRLoader::kHalfFloatType) == &loader);

    // A derived instance carries its own copy of the setting.
    RGBELoader rgb;    
    REQUIRE(rgb.dataType() == HDRLoader::kHalfFloatType);
}

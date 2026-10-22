#include "loaders/MTLLoader.h"

#include "catch.hpp"

#include <string>

namespace {
MTLLoader::MaterialInfo makeInfo() {
    MTLLoader::MaterialInfo info;
    info["name"] = std::string("Body");
    info["kd"] = MTLLoader::ColorValue{1, 0, 0};
    info["ks"] = MTLLoader::ColorValue{0, 1, 0};
    info["ke"] = MTLLoader::ColorValue{0, 0, 1};
    info["ns"] = std::string("250");
    info["d"] = std::string("0.5");
    return info;
}
}  // namespace

TEST_CASE("MTLLoader material params take the colour keys") {
    const auto params =
        MTLLoader::createMaterialParams(makeInfo(), MTLLoader::MaterialOptions{});
    REQUIRE(params.name == "Body");
    REQUIRE(params.hasColor);
    REQUIRE(params.hasSpecular);
    REQUIRE(params.hasEmissive);
    // The values are converted from sRGB to the working space, so they are not the raw bytes
    // over 255; what matters is that they were read and are not black.
    REQUIRE(params.color.r > 0.0);
}

TEST_CASE("MTLLoader material params read the scalar rules") {
    const auto params =
        MTLLoader::createMaterialParams(makeInfo(), MTLLoader::MaterialOptions{});
    REQUIRE(params.shininess == 250.0);
    // d below one sets both the opacity and the transparent flag.
    REQUIRE(params.opacity == Approx(0.5).epsilon(1e-12));
    REQUIRE(params.transparent);

    MTLLoader::MaterialInfo kept;
    kept["d"] = std::string("1");
    const auto opaque =
        MTLLoader::createMaterialParams(kept, MTLLoader::MaterialOptions{});
    REQUIRE_FALSE(opaque.transparent);
    REQUIRE(opaque.opacity == 1.0);
}

TEST_CASE("MTLLoader material params invert tr into opacity") {
    MTLLoader::MaterialInfo info;
    info["tr"] = std::string("0.3");
    const auto params =
        MTLLoader::createMaterialParams(info, MTLLoader::MaterialOptions{});
    // tr of 0.3 becomes an opacity of 0.7.
    REQUIRE(params.opacity == Approx(0.7).epsilon(1e-12));
    REQUIRE(params.transparent);

    // The invert option flips the tr value before the same arithmetic.
    MTLLoader::MaterialOptions options;
    options.invertTrProperty = true;
    const auto inverted = MTLLoader::createMaterialParams(info, options);
    REQUIRE(inverted.opacity == Approx(0.3).epsilon(1e-12));
}

TEST_CASE("MTLLoader material params keep the first texture per slot") {
    MTLLoader::MaterialInfo info;
    info["map_kd"] = std::string("first.png");
    info["map_ks"] = std::string("second.png");
    info["map_d"] = std::string("alpha.png");
    const auto params =
        MTLLoader::createMaterialParams(info, MTLLoader::MaterialOptions{});
    REQUIRE(params.map == "first.png");
    REQUIRE(params.specularMap == "second.png");
    // An alpha map forces transparency on its own.
    REQUIRE(params.alphaMap == "alpha.png");
    REQUIRE(params.transparent);
}

TEST_CASE("MTLLoader material params lift texture options into the params") {
    MTLLoader::MaterialInfo info;
    info["map_bump"] = std::string("bump.png -bm 2");
    info["disp"] = std::string("disp.png -mm 0.1 3");
    const auto params =
        MTLLoader::createMaterialParams(info, MTLLoader::MaterialOptions{});
    REQUIRE(params.bumpMap == "bump.png");
    REQUIRE(params.bumpScale.has_value());
    REQUIRE(params.bumpScale.value() == 2.0);
    REQUIRE(params.displacementMap == "disp.png");
    REQUIRE(params.displacementBias.value() == Approx(0.1).epsilon(1e-12));
    REQUIRE(params.displacementScale.value() == 3.0);
}

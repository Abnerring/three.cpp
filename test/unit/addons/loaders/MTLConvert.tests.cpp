#include "loaders/MTLLoader.h"

#include "catch.hpp"

#include <string>

namespace {
MTLLoader::MaterialsInfo makeInfo() {
    MTLLoader::MaterialInfo red;
    red["name"] = std::string("Red");
    red["kd"] = MTLLoader::ColorValue{255, 128, 0};
    red["ks"] = MTLLoader::ColorValue{0, 0, 0};
    red["ns"] = std::string("250.0");
    MTLLoader::MaterialsInfo info;
    info["Red"] = red;
    return info;
}
}  // namespace

TEST_CASE("MTLLoader convert keeps colours as they are by default") {
    MTLLoader::MaterialOptions options;
    const auto converted = MTLLoader::convert(makeInfo(), options);
    const auto kd = std::get<MTLLoader::ColorValue>(converted.at("Red").at("kd"));
    REQUIRE(kd[0] == 255.0);
    REQUIRE(kd[1] == 128.0);
    REQUIRE(kd[2] == 0.0);
    // A zero colour is kept when the ignore option is off.
    REQUIRE(converted.at("Red").count("ks") == 1);
    // Non colour values pass through untouched.
    REQUIRE(std::get<std::string>(converted.at("Red").at("ns")) == "250.0");
}

TEST_CASE("MTLLoader convert normalises colours when asked") {
    MTLLoader::MaterialOptions options;
    options.normalizeRGB = true;
    const auto converted = MTLLoader::convert(makeInfo(), options);
    const auto kd = std::get<MTLLoader::ColorValue>(converted.at("Red").at("kd"));
    REQUIRE(kd[0] == Approx(1.0).epsilon(1e-12));
    REQUIRE(kd[1] == Approx(128.0 / 255.0).epsilon(1e-12));
    REQUIRE(kd[2] == 0.0);
}

TEST_CASE("MTLLoader convert drops all zero colours when asked") {
    MTLLoader::MaterialOptions options;
    options.ignoreZeroRGBs = true;
    const auto converted = MTLLoader::convert(makeInfo(), options);
    // Ks is 0 0 0, so it is dropped entirely rather than stored as a zero colour.
    REQUIRE(converted.at("Red").count("ks") == 0);
    REQUIRE(converted.at("Red").count("kd") == 1);
}

TEST_CASE("MTLLoader convert lowercases the output keys") {
    MTLLoader::MaterialInfo info;
    info["Kd"] = MTLLoader::ColorValue{1, 2, 3};
    info["Ns"] = std::string("10");
    MTLLoader::MaterialsInfo materials;
    materials["Body"] = info;
    const auto converted = MTLLoader::convert(materials, MTLLoader::MaterialOptions{});
    REQUIRE(converted.at("Body").count("Kd") == 0);
    REQUIRE(converted.at("Body").count("kd") == 1);
    REQUIRE(converted.at("Body").count("ns") == 1);
}

TEST_CASE("MTLLoader material options carry the documented defaults") {
    const MTLLoader::MaterialOptions options;
    REQUIRE(options.side == FrontSide);
    REQUIRE(options.wrap == RepeatWrapping);
    REQUIRE_FALSE(options.normalizeRGB);
    REQUIRE_FALSE(options.ignoreZeroRGBs);
}

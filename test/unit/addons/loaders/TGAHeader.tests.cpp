#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
std::vector<std::uint8_t> makeBytes() {
    std::vector<std::uint8_t> data(18, 0);
    data[0] = 7;    // id length
    data[1] = 1;    // colour map present
    data[2] = 2;    // true colour
    data[3] = 0x01; // colormap first, little endian
    data[4] = 0x00;
    data[5] = 0x00; // colormap length 256
    data[6] = 0x01;
    data[7] = 24;   // colormap entry size
    data[8] = 3;    // x origin
    data[9] = 0;
    data[10] = 4;   // y origin
    data[11] = 0;
    data[12] = 0x40; // width 64
    data[13] = 0x00;
    data[14] = 0x20; // height 32
    data[15] = 0x00;
    data[16] = 32;  // pixel depth
    data[17] = 0x28; // descriptor
    return data;
}
}  // namespace

TEST_CASE("TGALoader reads the eighteen byte header") {
    const auto header = TGALoader::readHeader(makeBytes());
    REQUIRE(header.idLength == 7);
    REQUIRE(header.colormapType == 1);
    REQUIRE(header.imageType == TGALoader::kTypeRgb);
    REQUIRE(header.colormapFirst == 1);
    REQUIRE(header.colormapLength == 256);
    REQUIRE(header.colormapSize == 24);
    REQUIRE(header.xOrigin == 3);
    REQUIRE(header.yOrigin == 4);
    REQUIRE(header.width == 64);
    REQUIRE(header.height == 32);
    REQUIRE(header.pixelSize == 32);
    REQUIRE(header.descriptor == 0x28);
}

TEST_CASE("TGALoader reads the multi byte fields little endian") {
    auto data = makeBytes();
    // A width of 0x1234 stored little endian puts the low byte first.
    data[12] = 0x34;
    data[13] = 0x12;
    const auto header = TGALoader::readHeader(data);
    REQUIRE(header.width == 0x1234);
}

TEST_CASE("TGALoader rejects a header that is too short") {
    const std::vector<std::uint8_t> tiny(17, 0);
    REQUIRE_THROWS_WITH(TGALoader::readHeader(tiny), "THREE.TGALoader: Invalid header length.");
}

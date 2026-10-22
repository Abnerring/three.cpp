#include "loaders/MD2Loader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void pushInt32LE(std::vector<std::uint8_t>& out, std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    out.push_back(std::uint8_t(raw & 0xff));
    out.push_back(std::uint8_t((raw >> 8) & 0xff));
    out.push_back(std::uint8_t((raw >> 16) & 0xff));
    out.push_back(std::uint8_t((raw >> 24) & 0xff));
}

void pushInt16LE(std::vector<std::uint8_t>& out, std::int16_t value) {
    const std::uint16_t raw = static_cast<std::uint16_t>(value);
    out.push_back(std::uint8_t(raw & 0xff));
    out.push_back(std::uint8_t((raw >> 8) & 0xff));
}

// A header with the magic, version eight, a 64 by 32 skin and two texture coordinates.
std::vector<std::uint8_t> makeFile() {
    std::vector<std::uint8_t> data;
    pushInt32LE(data, 844121161);
    pushInt32LE(data, 8);
    pushInt32LE(data, 64);   // skinwidth
    pushInt32LE(data, 32);   // skinheight
    pushInt32LE(data, 0);    // framesize
    pushInt32LE(data, 0);    // num_skins
    pushInt32LE(data, 0);    // num_vertices
    pushInt32LE(data, 2);    // num_st
    pushInt32LE(data, 0);    // num_tris
    pushInt32LE(data, 0);    // num_glcmds
    pushInt32LE(data, 0);    // num_frames
    pushInt32LE(data, 0);    // offset_skins
    pushInt32LE(data, 68);   // offset_st, right after the header
    pushInt32LE(data, 0);    // offset_tris
    pushInt32LE(data, 0);    // offset_frames
    pushInt32LE(data, 0);    // offset_glcmds
    // offset_end filled in after the payload.
    const std::size_t endIndex = data.size();
    pushInt32LE(data, 0);
    // Two texture coordinates: (0, 0) and (64, 32).
    pushInt16LE(data, 0);
    pushInt16LE(data, 0);
    pushInt16LE(data, 64);
    pushInt16LE(data, 32);
    const std::int32_t end = static_cast<std::int32_t>(data.size());
    data[endIndex + 0] = std::uint8_t(end & 0xff);
    data[endIndex + 1] = std::uint8_t((end >> 8) & 0xff);
    data[endIndex + 2] = std::uint8_t((end >> 16) & 0xff);
    data[endIndex + 3] = std::uint8_t((end >> 24) & 0xff);
    return data;
}
}  // namespace

TEST_CASE("MD2Loader reads all seventeen header fields") {
    const auto header = MD2Loader::parseHeader(makeFile());
    REQUIRE(header.ident == 844121161);
    REQUIRE(header.version == 8);
    REQUIRE(header.skinwidth == 64);
    REQUIRE(header.skinheight == 32);
    REQUIRE(header.num_st == 2);
    REQUIRE(header.offset_st == 68);
    REQUIRE(MD2Loader::isValid(header));
}

TEST_CASE("MD2Loader rejects a wrong magic or version") {
    auto wrongMagic = makeFile();
    wrongMagic[0] = 0;
    const auto header = MD2Loader::parseHeader(wrongMagic);
    REQUIRE_FALSE(MD2Loader::isValid(header));

    auto wrongVersion = makeFile();
    wrongVersion[4] = 7;
    REQUIRE_FALSE(MD2Loader::isValid(MD2Loader::parseHeader(wrongVersion)));
}

TEST_CASE("MD2Loader checks the end offset against the file length") {
    const auto data = makeFile();
    const auto header = MD2Loader::parseHeader(data);
    REQUIRE(MD2Loader::isConsistent(header, data.size()));
    // A truncated file no longer matches the recorded end.
    REQUIRE_FALSE(MD2Loader::isConsistent(header, data.size() - 1));
}

TEST_CASE("MD2Loader normalises texture coordinates and flips v") {
    const auto data = makeFile();
    const auto header = MD2Loader::parseHeader(data);
    const auto uvs = MD2Loader::parseUVs(data, header);
    REQUIRE(uvs.size() == 4);
    // (0, 0) stays at the origin.
    REQUIRE(uvs[0] == 0.0);
    REQUIRE(uvs[1] == 1.0);
    // (64, 32) is the far corner, so u is one and v flips to zero.
    REQUIRE(uvs[2] == Approx(1.0).epsilon(1e-12));
    REQUIRE(uvs[3] == Approx(0.0).margin(1e-12));
}

TEST_CASE("MD2Loader fails when the header is short") {
    const std::vector<std::uint8_t> tiny{1, 2, 3};
    REQUIRE_THROWS_WITH(MD2Loader::parseHeader(tiny), "THREE.MD2Loader: Unexpected end of data");
}

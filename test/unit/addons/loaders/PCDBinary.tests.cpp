#include "loaders/PCDLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void pushFloat32(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
}
void pushUint32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}

PCDLoader::Header makeHeader() {
    PCDLoader::Header header;
    header.fields = {"x", "y", "z", "rgb"};
    header.size = {4, 4, 4, 4};
    header.type = {"f", "f", "f", "u"};
    header.count = {1, 1, 1, 1};
    header.data = "binary";
    return header;
}
}  // namespace

TEST_CASE("PCDLoader reads a binary row as floats and a packed colour") {
    std::vector<std::uint8_t> data;
    pushFloat32(data, 1.0f);
    pushFloat32(data, 2.0f);
    pushFloat32(data, 3.0f);
    pushUint32(data, 0x00ff0000u);  // pure red

    const auto header = makeHeader();
    const auto layout = PCDLoader::computeOffsets(header);
    REQUIRE(layout.rowSize == 16);
    const auto points = PCDLoader::parseBinaryPoints(data, 0, 1, header, layout, true);
    REQUIRE(points.positions.size() == 3);
    REQUIRE(points.positions[0] == 1.0);
    REQUIRE(points.positions[1] == 2.0);
    REQUIRE(points.positions[2] == 3.0);
    // The colour is unpacked and converted, so red survives and the others vanish.
    REQUIRE(points.colors.size() == 3);
    REQUIRE(points.colors[0] == Approx(1.0).margin(1e-6));
    REQUIRE(points.colors[1] == Approx(0.0).margin(1e-6));
    REQUIRE(points.colors[2] == Approx(0.0).margin(1e-6));
}

TEST_CASE("PCDLoader reads rows at the row stride") {
    std::vector<std::uint8_t> data;
    for (int i = 0; i < 2; ++i) {
        pushFloat32(data, float(i));
        pushFloat32(data, float(i + 10));
        pushFloat32(data, float(i + 20));
        pushUint32(data, 0x00ffffffu);
    }
    const auto header = makeHeader();
    const auto layout = PCDLoader::computeOffsets(header);
    const auto points = PCDLoader::parseBinaryPoints(data, 0, 2, header, layout, true);
    REQUIRE(points.positions.size() == 6);
    // The second row starts sixteen bytes in.
    REQUIRE(points.positions[3] == 1.0);
    REQUIRE(points.positions[5] == 21.0);
    REQUIRE(points.colors.size() == 6);
}

TEST_CASE("PCDLoader respects the requested endianness") {
    // All three position fields are declared, so the row really produces a position.
    PCDLoader::Header header;
    header.fields = {"x", "y", "z"};
    header.size = {4, 4, 4};
    header.type = {"f", "f", "f"};
    header.count = {1, 1, 1};
    header.data = "binary";
    const auto layout = PCDLoader::computeOffsets(header);

    // 1.0, 2.0 and 3.0 written big endian.
    const std::vector<std::uint8_t> big{0x3f, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
                                        0x40, 0x40, 0x00, 0x00};
    const auto bigRead = PCDLoader::parseBinaryPoints(big, 0, 1, header, layout, false);
    REQUIRE(bigRead.positions.size() == 3);
    REQUIRE(bigRead.positions[0] == 1.0);
    REQUIRE(bigRead.positions[1] == 2.0);
    REQUIRE(bigRead.positions[2] == 3.0);

    // The same values written little endian read back identically with the flag on.
    const std::vector<std::uint8_t> little{0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x40,
                                           0x00, 0x00, 0x40, 0x40};
    const auto littleRead = PCDLoader::parseBinaryPoints(little, 0, 1, header, layout, true);
    REQUIRE(littleRead.positions.size() == 3);
    REQUIRE(littleRead.positions[0] == 1.0);
    REQUIRE(littleRead.positions[2] == 3.0);
}

TEST_CASE("PCDLoader stops when a row runs past the data") {
    std::vector<std::uint8_t> data;
    pushFloat32(data, 1.0f);
    pushFloat32(data, 2.0f);
    pushFloat32(data, 3.0f);
    // No fourth field, so the colour read fails and the row still yields its position.
    const auto header = makeHeader();
    const auto layout = PCDLoader::computeOffsets(header);
    const auto points = PCDLoader::parseBinaryPoints(data, 0, 1, header, layout, true);
    REQUIRE(points.positions.size() == 3);
    REQUIRE(points.colors.empty());
}
TEST_CASE("PCDLoader reads a compressed block after its two lengths") {
    // Compress three floats with literal runs only, which the decompressor accepts.
    std::vector<std::uint8_t> plain;
    for (const float value : {1.0f, 2.0f, 3.0f}) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (int i = 0; i < 4; ++i) plain.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
    }
    // Twelve literal bytes in one run: control byte eleven, then the bytes.
    std::vector<std::uint8_t> compressed;
    compressed.push_back(std::uint8_t(plain.size() - 1));
    for (const auto byte : plain) compressed.push_back(byte);

    std::vector<std::uint8_t> block;
    const auto pushLength = [&block](std::size_t value) {
        for (int i = 0; i < 4; ++i) block.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
    };
    pushLength(compressed.size());
    pushLength(plain.size());
    for (const auto byte : compressed) block.push_back(byte);

    PCDLoader::Header header;
    header.fields = {"x", "y", "z"};
    header.size = {4, 4, 4};
    header.type = {"f", "f", "f"};
    header.count = {1, 1, 1};
    header.data = "binary_compressed";
    const auto layout = PCDLoader::computeOffsets(header);

    const auto points = PCDLoader::parseCompressedPoints(block, 0, 1, header, layout, true);
    REQUIRE(points.positions.size() == 3);
    REQUIRE(points.positions[0] == 1.0);
    REQUIRE(points.positions[1] == 2.0);
    REQUIRE(points.positions[2] == 3.0);
}

TEST_CASE("PCDLoader rejects a compressed block that is too short") {
    const std::vector<std::uint8_t> tiny{1, 2, 3};
    PCDLoader::Header header;
    header.fields = {"x"};
    header.size = {4};
    header.type = {"f"};
    header.count = {1};
    header.data = "binary_compressed";
    const auto layout = PCDLoader::computeOffsets(header);
    REQUIRE_THROWS_WITH(
        PCDLoader::parseCompressedPoints(tiny, 0, 1, header, layout, true),
        "THREE.PCDLoader: Invalid compressed data");
}

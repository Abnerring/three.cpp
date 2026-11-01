#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
void pushId(std::vector<std::uint8_t>& out, const std::string& id) {
    for (const char c : id) out.push_back(static_cast<std::uint8_t>(c));
}
void pushUint32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}

// Builds a file holding one chunk with the given content, parsed and its bytes.
struct Built {
    std::vector<std::uint8_t> bytes;
    VOXLoader::File file;
};
Built makeFile(const std::string& id, const std::vector<std::uint8_t>& content) {
    Built built;
    pushId(built.bytes, "VOX ");
    pushUint32(built.bytes, 150);
    pushId(built.bytes, id);
    pushUint32(built.bytes, static_cast<std::uint32_t>(content.size()));
    pushUint32(built.bytes, 0);
    for (const auto byte : content) built.bytes.push_back(byte);
    built.file = VOXLoader::parseChunks(built.bytes);
    return built;
}
}  // namespace

TEST_CASE("VOXLoader reads the size chunk") {
    std::vector<std::uint8_t> content;
    pushUint32(content, 4);
    pushUint32(content, 5);
    pushUint32(content, 6);
    const auto built = makeFile("SIZE", content);
    const auto size = VOXLoader::readSize(built.bytes, built.file.chunks[0]);
    REQUIRE(size.x == 4);
    REQUIRE(size.y == 5);
    REQUIRE(size.z == 6);
}

TEST_CASE("VOXLoader reads the voxel list") {
    std::vector<std::uint8_t> content;
    pushUint32(content, 2);
    // Two voxels: coordinates then a palette index.
    content.insert(content.end(), {1, 2, 3, 10});
    content.insert(content.end(), {4, 5, 6, 20});
    const auto built = makeFile("XYZI", content);
    const auto voxels = VOXLoader::readXyzi(built.bytes, built.file.chunks[0]);
    REQUIRE(voxels.size() == 2);
    REQUIRE(voxels[0].x == 1);
    REQUIRE(voxels[0].z == 3);
    REQUIRE(voxels[0].colorIndex == 10);
    REQUIRE(voxels[1].y == 5);
    REQUIRE(voxels[1].colorIndex == 20);
}

TEST_CASE("VOXLoader reads an empty voxel list") {
    std::vector<std::uint8_t> content;
    pushUint32(content, 0);
    const auto built = makeFile("XYZI", content);
    REQUIRE(VOXLoader::readXyzi(built.bytes, built.file.chunks[0]).empty());
}

TEST_CASE("VOXLoader reads the palette") {
    std::vector<std::uint8_t> content;
    content.insert(content.end(), {255, 0, 0, 255});
    content.insert(content.end(), {0, 255, 0, 128});
    const auto built = makeFile("RGBA", content);
    const auto palette = VOXLoader::readRgba(built.bytes, built.file.chunks[0]);
    REQUIRE(palette.size() == 2);
    REQUIRE(palette[0][0] == 255);
    REQUIRE(palette[0][3] == 255);
    // The second entry keeps its alpha.
    REQUIRE(palette[1][1] == 255);
    REQUIRE(palette[1][3] == 128);
}

TEST_CASE("VOXLoader rejects short content chunks") {
    const auto size = makeFile("SIZE", {1, 2});
    REQUIRE_THROWS_WITH(VOXLoader::readSize(size.bytes, size.file.chunks[0]),
                        "THREE.VOXLoader: SIZE chunk is too short");

    // A count of three with room for only one voxel is rejected too.
    std::vector<std::uint8_t> content;
    pushUint32(content, 3);
    content.insert(content.end(), {1, 1, 1, 1});
    const auto xyzi = makeFile("XYZI", content);
    REQUIRE_THROWS_WITH(VOXLoader::readXyzi(xyzi.bytes, xyzi.file.chunks[0]),
                        "THREE.VOXLoader: XYZI chunk is too short");
}

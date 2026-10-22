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
}  // namespace

TEST_CASE("VOXLoader splits a nested chunk tree") {
    // Two child chunks inside MAIN: a three byte SIZE and a four byte XYZI.
    std::vector<std::uint8_t> children;
    pushId(children, "SIZE");
    pushUint32(children, 3);
    pushUint32(children, 0);
    children.push_back(2);
    children.push_back(3);
    children.push_back(4);

    pushId(children, "XYZI");
    pushUint32(children, 4);
    pushUint32(children, 0);
    for (int i = 0; i < 4; ++i) children.push_back(std::uint8_t(i));

    std::vector<std::uint8_t> file;
    pushId(file, "VOX ");
    pushUint32(file, 150);
    pushId(file, "MAIN");
    pushUint32(file, 0);
    pushUint32(file, static_cast<std::uint32_t>(children.size()));
    for (const auto byte : children) file.push_back(byte);

    const auto parsed = VOXLoader::parseChunks(file);
    REQUIRE(parsed.version == 150);
    REQUIRE(parsed.chunks.size() == 1);
    REQUIRE(parsed.chunks[0].id == "MAIN");
    // MAIN has no content of its own but two children.
    REQUIRE(parsed.chunks[0].contentSize == 0);
    REQUIRE(parsed.chunks[0].children.size() == 2);
    REQUIRE(parsed.chunks[0].children[0].id == "SIZE");
    REQUIRE(parsed.chunks[0].children[0].contentSize == 3);
    REQUIRE(parsed.chunks[0].children[1].id == "XYZI");
    REQUIRE(parsed.chunks[0].children[1].contentSize == 4);
    // The content offset points at the bytes just after the header.
    const auto offset = parsed.chunks[0].children[1].contentOffset;
    REQUIRE(file[offset] == 0);
    REQUIRE(file[offset + 3] == 3);
}

TEST_CASE("VOXLoader reads two top level chunks") {
    std::vector<std::uint8_t> file;
    pushId(file, "VOX ");
    pushUint32(file, 200);
    // A first chunk with one content byte and no children.
    pushId(file, "SIZE");
    pushUint32(file, 1);
    pushUint32(file, 0);
    file.push_back(9);
    // A second chunk with no content at all.
    pushId(file, "RGBA");
    pushUint32(file, 0);
    pushUint32(file, 0);

    const auto parsed = VOXLoader::parseChunks(file);
    REQUIRE(parsed.version == 200);
    REQUIRE(parsed.chunks.size() == 2);
    REQUIRE(parsed.chunks[0].contentSize == 1);
    REQUIRE(parsed.chunks[1].id == "RGBA");
}

TEST_CASE("VOXLoader rejects a file without the signature") {
    const std::vector<std::uint8_t> wrong{'N', 'O', 'P', 'E', 1, 0, 0, 0};
    REQUIRE_THROWS_WITH(VOXLoader::parseChunks(wrong), "THREE.VOXLoader: Not a VOX file");
    // A buffer too short to hold a signature is not one either.
    REQUIRE_THROWS_WITH(VOXLoader::parseChunks({1, 2, 3}), "THREE.VOXLoader: Not a VOX file");
}

TEST_CASE("VOXLoader rejects a chunk that runs past the file") {
    std::vector<std::uint8_t> file;
    pushId(file, "VOX ");
    pushUint32(file, 150);
    pushId(file, "SIZE");
    // The chunk claims ten bytes but none follow.
    pushUint32(file, 10);
    pushUint32(file, 0);
    REQUIRE_THROWS_WITH(VOXLoader::parseChunks(file), "THREE.VOXLoader: Chunk is larger than the file");
}

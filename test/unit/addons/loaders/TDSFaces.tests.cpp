#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
// A face array chunk, laid out the way the format does: two bytes of identifier, four bytes of
// little endian length, then the contents.
//
//   id:     40 41           (0x4140, low byte first)
//   length: 18 00 00 00     (twenty four, covering the header, the count and eight words)
//   count:  02 00           (two faces)
//   face 0: 00 00 01 00 02 00 05 00
//   face 1: 02 00 03 00 00 00 07 00
std::vector<std::uint8_t> faceChunk() {
    return {0x40, 0x41, 0x18, 0x00, 0x00, 0x00,
            0x02, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x05, 0x00,
            0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00};
}
}  // namespace

TEST_CASE("TDSLoader reads a face array") {
    const auto buffer = faceChunk();
    // The declared length covers the header, the count and the eight words.
    REQUIRE(buffer.size() == 24);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(chunk.id == 0x4140);
    REQUIRE(chunk.size == 24);

    const auto faces = TDSLoader::readFaces(buffer, chunk);
    REQUIRE(faces.faceCount == 2);
    // Two faces of three nodes, with the visibility words dropped.
    REQUIRE(faces.indices.size() == 6);
    REQUIRE(faces.indices[0] == 0);
    REQUIRE(faces.indices[2] == 2);
    REQUIRE(faces.indices[3] == 2);
    REQUIRE(faces.indices[4] == 3);
    REQUIRE(faces.indices[5] == 0);
    // The cursor has reached the end of the chunk, so no sub chunks follow.
    REQUIRE(TDSLoader::atEnd(chunk));
}

TEST_CASE("TDSLoader drops the visibility word") {
    const auto buffer = faceChunk();
    auto chunk = TDSLoader::readChunk(buffer, 0);
    const auto faces = TDSLoader::readFaces(buffer, chunk);
    // Five and seven were the visibility words and must not appear among the indices.
    for (const auto value : faces.indices) {
        REQUIRE(value != 5);
        REQUIRE(value != 7);
    }
}

TEST_CASE("TDSLoader lays out the material groups") {
    // Three groups of two, one and four faces.
    const auto ranges = TDSLoader::groupRanges({2, 1, 4});
    REQUIRE(ranges.size() == 3);
    // The first starts at zero and covers six indices.
    REQUIRE(ranges[0].start == 0);
    REQUIRE(ranges[0].count == 6);
    REQUIRE(ranges[0].materialIndex == 0);
    // The second follows it.
    REQUIRE(ranges[1].start == 6);
    REQUIRE(ranges[1].count == 3);
    REQUIRE(ranges[1].materialIndex == 1);
    // The third follows that.
    REQUIRE(ranges[2].start == 9);
    REQUIRE(ranges[2].count == 12);
    REQUIRE(ranges[2].materialIndex == 2);
}

TEST_CASE("TDSLoader handles a file with no groups") {
    REQUIRE(TDSLoader::groupRanges({}).empty());
    // One group still starts at zero.
    const auto one = TDSLoader::groupRanges({1});
    REQUIRE(one.size() == 1);
    REQUIRE(one[0].start == 0);
    REQUIRE(one[0].count == 3);
}

TEST_CASE("TDSLoader reads no faces from a chunk that has none") {
    // A chunk of the header and a zero count: eight bytes, which is what it declares.
    std::vector<std::uint8_t> buffer{0x40, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    REQUIRE(buffer.size() == 8);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(chunk.id == 0x4140);
    const auto faces = TDSLoader::readFaces(buffer, chunk);
    REQUIRE(faces.faceCount == 0);
    REQUIRE(faces.indices.empty());
    REQUIRE(TDSLoader::atEnd(chunk));
}
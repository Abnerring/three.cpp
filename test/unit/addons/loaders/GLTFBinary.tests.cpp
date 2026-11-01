#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
void putU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}
// A twelve byte header with the given version and total length.
std::vector<std::uint8_t> header(std::uint32_t version, std::uint32_t length) {
    std::vector<std::uint8_t> out{'g', 'l', 'T', 'F'};
    putU32(out, version);
    putU32(out, length);
    return out;
}
// A chunk of the given type and payload, with its length filled in.
void addChunk(std::vector<std::uint8_t>& out, std::uint32_t type,
              const std::vector<std::uint8_t>& payload) {
    putU32(out, static_cast<std::uint32_t>(payload.size()));
    putU32(out, type);
    out.insert(out.end(), payload.begin(), payload.end());
}
}  // namespace

TEST_CASE("GLTFLoader reads a binary header") {
    const auto buffer = header(2, 12);
    REQUIRE(buffer.size() == 12);
    const auto parsed = GLTFLoader::readBinaryHeader(buffer);
    REQUIRE(parsed.magic == "glTF");
    REQUIRE(parsed.version == 2);
    REQUIRE(parsed.length == 12);
}

TEST_CASE("GLTFLoader refuses a wrong magic") {
    auto buffer = header(2, 12);
    buffer[0] = 'G';
    REQUIRE_THROWS_WITH(GLTFLoader::readBinaryHeader(buffer),
                        "THREE.GLTFLoader: Unsupported glTF-Binary header.");
    // A buffer too short for a header is refused the same way.
    REQUIRE_THROWS_WITH(GLTFLoader::readBinaryHeader({1, 2, 3}),
                        "THREE.GLTFLoader: Unsupported glTF-Binary header.");
}

TEST_CASE("GLTFLoader refuses the earlier version") {
    // Version one laid its chunks out differently.
    const auto buffer = header(1, 12);
    REQUIRE_THROWS_WITH(GLTFLoader::readBinaryHeader(buffer),
                        "THREE.GLTFLoader: Legacy binary file detected.");
    // Version two is accepted.
    REQUIRE(GLTFLoader::readBinaryHeader(header(2, 12)).version == 2);
}

TEST_CASE("GLTFLoader reads a json chunk") {
    // A header and one four byte chunk make twenty four bytes in all.
    auto buffer = header(2, 24);
    addChunk(buffer, GLTFLoader::kChunkJson, {1, 2, 3, 4});
    REQUIRE(buffer.size() == 24);
    const auto parsed = GLTFLoader::readBinaryHeader(buffer);
    const auto chunks = GLTFLoader::readBinaryChunks(buffer, parsed);
    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0].type == GLTFLoader::kChunkJson);
    REQUIRE(chunks[0].length == 4);
    // The payload begins after the eight bytes of length and type.
    REQUIRE(chunks[0].offset == 20);
}

TEST_CASE("GLTFLoader reads a json and a bin chunk") {
    // Two chunks of four bytes: the header, then eight of framing and four of payload each.
    auto buffer = header(2, 36);
    addChunk(buffer, GLTFLoader::kChunkJson, {'{', '}', ' ', ' '});
    addChunk(buffer, GLTFLoader::kChunkBin, {9, 8, 7, 6});
    REQUIRE(buffer.size() == 36);
    const auto chunks = GLTFLoader::readBinaryChunks(buffer, GLTFLoader::readBinaryHeader(buffer));
    REQUIRE(chunks.size() == 2);
    REQUIRE(chunks[0].type == GLTFLoader::kChunkJson);
    REQUIRE(chunks[1].type == GLTFLoader::kChunkBin);
    REQUIRE(chunks[1].offset == 32);
}

TEST_CASE("GLTFLoader ignores a chunk of an unknown type") {
    // A type the loader does not know is still walked past rather than refused.
    auto buffer = header(2, 24);
    addChunk(buffer, 0x12345678, {1, 2, 3, 4});
    REQUIRE(buffer.size() == 24);
    const auto chunks = GLTFLoader::readBinaryChunks(buffer, GLTFLoader::readBinaryHeader(buffer));
    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0].type == 0x12345678);
}

TEST_CASE("GLTFLoader reads no chunks from a header alone") {
    const auto buffer = header(2, 12);
    REQUIRE(GLTFLoader::readBinaryChunks(buffer, GLTFLoader::readBinaryHeader(buffer)).empty());
    // A declared length below the header leaves nothing either.
    const auto shortFile = header(2, 4);
    REQUIRE(GLTFLoader::readBinaryChunks(shortFile, GLTFLoader::readBinaryHeader(shortFile)).empty());
}

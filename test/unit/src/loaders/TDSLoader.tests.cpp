#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

// Every buffer below is exactly as long as the size word it declares, because the readers
// treat anything past the buffer as absent and return zero instead.

TEST_CASE("TDSLoader reads a chunk header") {
    // MM with a declared length of sixteen, and sixteen bytes in all.
    std::vector<std::uint8_t> buffer(16, 0);
    buffer[0] = 0x4D; buffer[1] = 0x4D;
    buffer[2] = 0x10;
    const auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(chunk.id == 0x4D4D);
    REQUIRE(chunk.size == 16);
    REQUIRE(chunk.offset == 0);
    // The cursor starts just past the six byte header.
    REQUIRE(chunk.position == 6);
    REQUIRE(chunk.end == 16);
    REQUIRE_FALSE(TDSLoader::atEnd(chunk));
}

TEST_CASE("TDSLoader reads little endian numbers") {
    // Header, a word and a float: twelve bytes, which is the declared length.
    std::vector<std::uint8_t> buffer{0x4D, 0x4D, 0x0C, 0x00, 0x00, 0x00,
                                     0x34, 0x12, 0x00, 0x00, 0x00, 0x3F};
    REQUIRE(buffer.size() == 12);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readWord(buffer, chunk) == 0x1234);
    REQUIRE(chunk.position == 8);
    // A half is three F, three zeros in its high bits, written low byte first.
    REQUIRE(TDSLoader::readFloat(buffer, chunk) == 0.5f);
    REQUIRE(chunk.position == 12);
}

TEST_CASE("TDSLoader reads signed values") {
    // Header, a short and an int: twelve bytes again.
    std::vector<std::uint8_t> buffer{0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
                                     0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF};
    REQUIRE(buffer.size() == 12);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readShort(buffer, chunk) == -1);
    REQUIRE(TDSLoader::readInt(buffer, chunk) == -2);
}

TEST_CASE("TDSLoader reads a null terminated string") {
    // Header plus four characters and their terminator: eleven bytes.
    std::vector<std::uint8_t> buffer{0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
                                     'M', 'e', 's', 'h', 0x00};
    REQUIRE(buffer.size() == 11);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readString(buffer, chunk) == "Mesh");
    // The terminator was consumed, so the cursor reached the very end.
    REQUIRE(chunk.position == 11);
}

TEST_CASE("TDSLoader reports the end of a chunk") {
    // A chunk of exactly its own header is already finished.
    std::vector<std::uint8_t> buffer{0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
    const auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(chunk.size == 6);
    REQUIRE(chunk.position == 6);
    REQUIRE(TDSLoader::atEnd(chunk));
}

TEST_CASE("TDSLoader rejects a buffer too short for a header") {
    // Five bytes cannot hold the six byte header.
    const std::vector<std::uint8_t> tiny{1, 2, 3, 4, 5};
    REQUIRE_THROWS_WITH(TDSLoader::readChunk(tiny, 0),
                        "THREE.TDSLoader: Invalid chunk header.");
}

TEST_CASE("TDSLoader reads zero past the end") {
    std::vector<std::uint8_t> buffer{0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
    auto chunk = TDSLoader::readChunk(buffer, 0);
    // Nothing follows the header, so a read returns zero and leaves the cursor alone.
    REQUIRE(TDSLoader::readDWord(buffer, chunk) == 0);
    REQUIRE(chunk.position == 6);
}

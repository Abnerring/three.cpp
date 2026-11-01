#include "loaders/LWOLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
// A chunk header of the given identifier and big endian size.
std::vector<std::uint8_t> chunk(const char* id, std::uint32_t size) {
    std::vector<std::uint8_t> out(8, 0);
    for (int i = 0; i < 4; ++i) out[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(id[i]);
    for (int i = 0; i < 4; ++i) {
        out[4 + i] = std::uint8_t((size >> ((3 - i) * 8)) & 0xff);
    }
    return out;
}
}  // namespace

TEST_CASE("LWOLoader reads a chunk header") {
    const auto buffer = chunk("FORM", 0x1234);
    const auto header = LWOLoader::readChunkHeader(buffer);
    REQUIRE(header.id == "FORM");
    // The count is big endian.
    REQUIRE(header.size == 0x1234);
    REQUIRE(header.dataOffset == 8);
}

TEST_CASE("LWOLoader trims the padding of a short identifier") {
    // A two character name is padded with spaces.
    const auto buffer = chunk("AB  ", 7);
    REQUIRE(LWOLoader::readChunkHeader(buffer).id == "AB");
}

TEST_CASE("LWOLoader reads a header at an offset") {
    std::vector<std::uint8_t> buffer(4, 0);
    const auto inner = chunk("LWO2", 100);
    buffer.insert(buffer.end(), inner.begin(), inner.end());
    const auto header = LWOLoader::readChunkHeader(buffer, 4);
    REQUIRE(header.id == "LWO2");
    REQUIRE(header.size == 100);
    REQUIRE(header.dataOffset == 12);
}

TEST_CASE("LWOLoader rejects a buffer too short for a header") {
    const std::vector<std::uint8_t> tiny{1, 2, 3};
    REQUIRE_THROWS_WITH(LWOLoader::readChunkHeader(tiny),
                        "THREE.LWOLoader: Invalid chunk header.");
}

TEST_CASE("LWOLoader cuts the parent out of a url") {
    // Everything before the directory is the parent.
    REQUIRE(LWOLoader::extractParentUrl("models/obj/file.lwo", "obj") == "models/");
    // A directory that never appears gives the current one.
    REQUIRE(LWOLoader::extractParentUrl("models/file.lwo", "textures") == "./");
    // One at the very front leaves nothing before it.
    REQUIRE(LWOLoader::extractParentUrl("obj/file.lwo", "obj").empty());
}

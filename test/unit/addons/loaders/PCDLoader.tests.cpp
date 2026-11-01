#include "loaders/PCDLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("PCDLoader decompresses a literal run") {
    // Control byte two means three literal bytes follow.
    const std::vector<std::uint8_t> data{2, 'a', 'b', 'c'};
    const auto out = PCDLoader::decompressLZF(data, 3);
    REQUIRE(out.size() == 3);
    REQUIRE(out[0] == 'a');
    REQUIRE(out[2] == 'c');
}

TEST_CASE("PCDLoader decompresses a back reference") {
    // One literal 'a', then a reference with length three at distance one, which copies
    // length plus two bytes from the byte just written.
    const std::vector<std::uint8_t> data{0, 'a', 0x60, 0x00};
    const auto out = PCDLoader::decompressLZF(data, 6);
    REQUIRE(out.size() == 6);
    for (const auto value : out) REQUIRE(value == 'a');
}

TEST_CASE("PCDLoader reports an undersized output buffer") {
    const std::vector<std::uint8_t> data{2, 'a', 'b', 'c'};
    REQUIRE_THROWS_WITH(PCDLoader::decompressLZF(data, 2),
                        "THREE.PCDLoader: Output buffer is not large enough");
}

TEST_CASE("PCDLoader reports truncated input") {
    // The control byte claims three bytes but only one follows.
    const std::vector<std::uint8_t> data{2, 'a'};
    REQUIRE_THROWS_WITH(PCDLoader::decompressLZF(data, 3),
                        "THREE.PCDLoader: Invalid compressed data");
}

TEST_CASE("PCDLoader reports a reference before the start") {
    // A reference whose offset points before the output start is invalid.
    const std::vector<std::uint8_t> data{0x20, 0xff};
    REQUIRE_THROWS_WITH(PCDLoader::decompressLZF(data, 8),
                        "THREE.PCDLoader: Invalid compressed data");
}

TEST_CASE("PCDLoader reads the ascii header") {
    const std::string text =
        "# .PCD v0.7\n"
        "VERSION 0.7\n"
        "FIELDS x y z rgb\n"
        "SIZE 4 4 4 4\n"
        "TYPE F F F U\n"
        "COUNT 1 1 1 1\n"
        "WIDTH 2\n"
        "HEIGHT 1\n"
        "POINTS 2\n"
        "DATA ascii\n";
    const std::vector<std::uint8_t> buffer(text.begin(), text.end());
    const auto header = PCDLoader::parseHeader(buffer);
    REQUIRE(header.fields.size() == 4);
    REQUIRE(header.fields[0] == "x");
    REQUIRE(header.fields[3] == "rgb");
    REQUIRE(header.size.size() == 4);
    REQUIRE(header.size[0] == 4);
    // The types keep their case as written, lower cased like the other string lists.
    REQUIRE(header.type[0] == "f");
    REQUIRE(header.type[3] == "u");
    REQUIRE(header.count.size() == 4);
    REQUIRE(header.width == 2);
    REQUIRE(header.height == 1);
    REQUIRE(header.points == 2);
    REQUIRE(header.data == "ascii");
    // The header length covers every byte up to and including the data line.
    REQUIRE(header.headerLength == buffer.size());
}

TEST_CASE("PCDLoader rejects a header without a data line") {
    const std::string text = "FIELDS x y z\nDATA\n";
    // The bare DATA line names no storage type.
    const std::vector<std::uint8_t> buffer(text.begin(), text.end());
    REQUIRE_THROWS_WITH(PCDLoader::parseHeader(buffer), "THREE.PCDLoader: Missing data declaration");
}

#include "loaders/PCDLoader.h"

#include "catch.hpp"

#include <string>

namespace {
PCDLoader::Header makeHeader(const std::string& data) {
    PCDLoader::Header header;
    header.fields = {"x", "y", "z", "rgb"};
    header.size = {4, 4, 4, 4};
    header.type = {"f", "f", "f", "u"};
    header.count = {1, 1, 1, 1};
    header.data = data;
    return header;
}
}  // namespace

TEST_CASE("PCDLoader numbers ascii fields by token") {
    const auto layout = PCDLoader::computeOffsets(makeHeader("ascii"));
    // One value per field, so the token offsets are 0, 1, 2 and 3.
    REQUIRE(layout.offset.at("x") == 0);
    REQUIRE(layout.offset.at("y") == 1);
    REQUIRE(layout.offset.at("z") == 2);
    REQUIRE(layout.offset.at("rgb") == 3);
    // Ascii has no row size.
    REQUIRE(layout.rowSize == 0);
}

TEST_CASE("PCDLoader numbers binary fields by byte") {
    const auto layout = PCDLoader::computeOffsets(makeHeader("binary"));
    // Four bytes per field, so the byte offsets step by four.
    REQUIRE(layout.offset.at("x") == 0);
    REQUIRE(layout.offset.at("y") == 4);
    REQUIRE(layout.offset.at("z") == 8);
    REQUIRE(layout.offset.at("rgb") == 12);
    REQUIRE(layout.rowSize == 16);
}

TEST_CASE("PCDLoader accounts for a field with a count above one") {
    auto binary = makeHeader("binary");
    binary.count = {1, 1, 1, 3};
    const auto layout = PCDLoader::computeOffsets(binary);
    // The last field still starts at twelve but the row grows by its three values.
    REQUIRE(layout.offset.at("rgb") == 12);
    REQUIRE(layout.rowSize == 24);

    auto text = makeHeader("ascii");
    text.count = {1, 1, 1, 3};
    const auto ascii = PCDLoader::computeOffsets(text);
    REQUIRE(ascii.offset.at("rgb") == 3);
    REQUIRE(ascii.rowSize == 0);
}

TEST_CASE("PCDLoader handles a compressed binary header like a binary one") {
    const auto layout = PCDLoader::computeOffsets(makeHeader("binary_compressed"));
    // Anything that is not ascii uses byte offsets and a row size.
    REQUIRE(layout.offset.at("y") == 4);
    REQUIRE(layout.rowSize == 16);
}

TEST_CASE("PCDLoader tolerates a header with fewer count and size entries") {
    auto header = makeHeader("binary");
    // A missing entry falls back to a size and count of one rather than reading past the end.
    // Only the first field carries the two by two metadata; the other three are one byte each.
    header.count = {2};
    header.size = {2};
    const auto layout = PCDLoader::computeOffsets(header);
    REQUIRE(layout.offset.at("x") == 0);
    REQUIRE(layout.offset.at("y") == 4);
    REQUIRE(layout.offset.at("z") == 5);
    REQUIRE(layout.rowSize == 7);
}

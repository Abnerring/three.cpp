#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
void pushUint32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}
void pushString(std::vector<std::uint8_t>& out, const std::string& text) {
    pushUint32(out, static_cast<std::uint32_t>(text.size()));
    for (const char c : text) out.push_back(static_cast<std::uint8_t>(c));
}
}  // namespace

TEST_CASE("VOXLoader reads a length prefixed string") {
    std::vector<std::uint8_t> buffer;
    pushString(buffer, "abc");
    const auto value = VOXLoader::readString(buffer, 0);
    REQUIRE(value.value == "abc");
    // Four length bytes plus three characters.
    REQUIRE(value.size == 7);
}

TEST_CASE("VOXLoader reads a dictionary") {
    std::vector<std::uint8_t> buffer;
    pushUint32(buffer, 2);
    pushString(buffer, "_name");
    pushString(buffer, "root");
    pushString(buffer, "_t");
    pushString(buffer, "0 1 2");
    std::size_t consumed = 0;
    const auto dict = VOXLoader::readDict(buffer, 0, consumed);
    REQUIRE(dict.size() == 2);
    REQUIRE(dict.at("_name") == "root");
    // Values may hold spaces, which is how a translation is written.
    REQUIRE(dict.at("_t") == "0 1 2");
    // The dictionary consumed the whole buffer.
    REQUIRE(consumed == buffer.size());
}

TEST_CASE("VOXLoader reads an empty dictionary") {
    std::vector<std::uint8_t> buffer;
    pushUint32(buffer, 0);
    std::size_t consumed = 0;
    const auto dict = VOXLoader::readDict(buffer, 0, consumed);
    REQUIRE(dict.empty());
    REQUIRE(consumed == 4);
}

TEST_CASE("VOXLoader rejects a string that runs past the buffer") {
    std::vector<std::uint8_t> buffer;
    // A length of five with only two characters behind it.
    pushUint32(buffer, 5);
    buffer.push_back('a');
    buffer.push_back('b');
    REQUIRE_THROWS_WITH(VOXLoader::readString(buffer, 0),
                        "THREE.VOXLoader: String runs past the chunk");
}
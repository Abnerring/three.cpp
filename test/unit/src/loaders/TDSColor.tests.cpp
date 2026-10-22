#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

// Each buffer is exactly as long as the outer size word declares, and each test says so.
namespace {
// An outer chunk of six header bytes followed by an inner chunk whose id and payload are given.
std::vector<std::uint8_t> wrapped(std::uint16_t innerId, std::vector<std::uint8_t> payload) {
    const std::uint32_t innerSize = static_cast<std::uint32_t>(6 + payload.size());
    const std::uint32_t outerSize = 6 + innerSize;
    std::vector<std::uint8_t> out;
    const auto put16 = [&out](std::uint16_t v) {
        out.push_back(std::uint8_t(v & 0xff));
        out.push_back(std::uint8_t((v >> 8) & 0xff));
    };
    const auto put32 = [&out](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((v >> (i * 8)) & 0xff));
    };
    put16(0x4000);
    put32(outerSize);
    put16(innerId);
    put32(innerSize);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}
std::vector<std::uint8_t> floats(std::initializer_list<float> values) {
    std::vector<std::uint8_t> out;
    for (float f : values) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &f, 4);
        for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
    }
    return out;
}
}  // namespace

TEST_CASE("TDSLoader reads a colour") {
    // A byte colour of ten, twenty, thirty: inner chunk of nine bytes, outer of fifteen.
    const auto buffer = wrapped(0x0011, {10, 20, 30});
    REQUIRE(buffer.size() == 15);
    auto parent = TDSLoader::readChunk(buffer, 0);
    const auto colour = TDSLoader::readColor(buffer, parent);
    REQUIRE(colour.known);
    REQUIRE(colour.r == Approx(10.0 / 255.0).epsilon(1e-9));
    REQUIRE(colour.b == Approx(30.0 / 255.0).epsilon(1e-9));
    // The parent moved past the whole sub chunk.
    REQUIRE(parent.position == 15);
}

TEST_CASE("TDSLoader reads a float colour") {
    // The floating form is taken as it stands, with no division by the byte range.
    const auto buffer = wrapped(0x0010, floats({0.25f, 0.5f, 0.75f}));
    // Twelve payload bytes, so inner is eighteen and outer is twenty four.
    REQUIRE(buffer.size() == 24);
    auto parent = TDSLoader::readChunk(buffer, 0);
    const auto colour = TDSLoader::readColor(buffer, parent);
    REQUIRE(colour.known);
    REQUIRE(colour.r == Approx(0.25).epsilon(1e-6));
    REQUIRE(colour.g == Approx(0.5).epsilon(1e-6));
    REQUIRE(colour.b == Approx(0.75).epsilon(1e-6));
}

TEST_CASE("TDSLoader leaves an unknown colour at white") {
    // An identifier the reader does not know returns the helper's default colour.
    const auto buffer = wrapped(0x0099, {1, 2, 3});
    REQUIRE(buffer.size() == 15);
    auto parent = TDSLoader::readChunk(buffer, 0);
    const auto colour = TDSLoader::readColor(buffer, parent);
    REQUIRE_FALSE(colour.known);
    REQUIRE(colour.r == 1.0);
    REQUIRE(colour.g == 1.0);
    REQUIRE(colour.b == 1.0);
}

TEST_CASE("TDSLoader reads a percentage") {
    // The integer form is a short over a hundred, so five thousand is fifty per cent.
    std::vector<std::uint8_t> buffer;
    const auto inner = wrapped(0x0030, {0x88, 0x13});
    buffer = inner;
    // Two payload bytes, so inner is eight and outer is fourteen.
    REQUIRE(buffer.size() == 14);
    auto parent = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readPercentage(buffer, parent) == Approx(50.0).epsilon(1e-9));
}

TEST_CASE("TDSLoader reads a float percentage") {
    const auto buffer = wrapped(0x0031, floats({0.125f}));
    // Four payload bytes, so inner is ten and outer is sixteen.
    REQUIRE(buffer.size() == 16);
    auto parent = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readPercentage(buffer, parent) == Approx(0.125).epsilon(1e-6));
}

TEST_CASE("TDSLoader reads an unknown percentage as zero") {
    const auto buffer = wrapped(0x0099, {0, 0});
    REQUIRE(buffer.size() == 14);
    auto parent = TDSLoader::readChunk(buffer, 0);
    REQUIRE(TDSLoader::readPercentage(buffer, parent) == 0.0);
}
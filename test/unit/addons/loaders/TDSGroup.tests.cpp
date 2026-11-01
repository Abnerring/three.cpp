#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

// Every buffer is exactly as long as its length word declares and each test says so. All words
// are little endian and the identifier comes first, which the previous rounds had to learn.
namespace {
std::vector<std::uint8_t> floats(std::initializer_list<float> values) {
    std::vector<std::uint8_t> out;
    for (float f : values) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &f, 4);
        for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
    }
    return out;
}
// A chunk with the given identifier and contents, with the length filled in.
std::vector<std::uint8_t> chunkWith(std::uint16_t id, const std::vector<std::uint8_t>& payload) {
    const std::uint32_t size = static_cast<std::uint32_t>(6 + payload.size());
    std::vector<std::uint8_t> out;
    out.push_back(std::uint8_t(id & 0xff));
    out.push_back(std::uint8_t((id >> 8) & 0xff));
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((size >> (i * 8)) & 0xff));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}
}  // namespace

TEST_CASE("TDSLoader reads a material group") {
    // The name "A" with its terminator, a count of two, and two face numbers.
    const auto buffer = chunkWith(0x4130, {'A', 0x00, 0x02, 0x00, 0x07, 0x00, 0x09, 0x00});
    REQUIRE(buffer.size() == 14);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    REQUIRE(chunk.id == 0x4130);

    const auto group = TDSLoader::readMaterialGroup(buffer, chunk);
    REQUIRE(group.name == "A");
    REQUIRE(group.index.size() == 2);
    REQUIRE(group.index[0] == 7);
    REQUIRE(group.index[1] == 9);
    REQUIRE(TDSLoader::atEnd(chunk));
}

TEST_CASE("TDSLoader reads a group with an empty name") {
    // A lone terminator names nothing, and a count of one gives a single face.
    const auto buffer = chunkWith(0x4130, {0x00, 0x01, 0x00, 0x03, 0x00});
    REQUIRE(buffer.size() == 11);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    const auto group = TDSLoader::readMaterialGroup(buffer, chunk);
    REQUIRE(group.name.empty());
    REQUIRE(group.index.size() == 1);
    REQUIRE(group.index[0] == 3);
}

TEST_CASE("TDSLoader reads the map values") {
    // Each map chunk holds one float, so each is ten bytes long.
    const auto offsetX = chunkWith(0xA358, floats({0.25f}));
    REQUIRE(offsetX.size() == 10);
    auto chunk = TDSLoader::readChunk(offsetX, 0);
    TDSLoader::MapTransform transform;
    TDSLoader::applyMapChunk(offsetX, chunk, transform);
    REQUIRE(transform.offsetX == Approx(0.25).epsilon(1e-6));
    // The other three keep their defaults.
    REQUIRE(transform.offsetY == 0.0);
    REQUIRE(transform.repeatX == 1.0);
    REQUIRE(transform.repeatY == 1.0);
}

TEST_CASE("TDSLoader fills in all four map values") {
    TDSLoader::MapTransform transform;
    const auto uo = chunkWith(0xA358, floats({0.5f}));
    const auto vo = chunkWith(0xA35A, floats({0.75f}));
    const auto us = chunkWith(0xA354, floats({2.0f}));
    const auto vs = chunkWith(0xA356, floats({3.0f}));
    for (const auto* buffer : {&uo, &vo, &us, &vs}) {
        auto chunk = TDSLoader::readChunk(*buffer, 0);
        TDSLoader::applyMapChunk(*buffer, chunk, transform);
    }
    REQUIRE(transform.offsetX == Approx(0.5).epsilon(1e-6));
    REQUIRE(transform.offsetY == Approx(0.75).epsilon(1e-6));
    REQUIRE(transform.repeatX == Approx(2.0).epsilon(1e-6));
    REQUIRE(transform.repeatY == Approx(3.0).epsilon(1e-6));
    REQUIRE(transform.name.empty());
}

TEST_CASE("TDSLoader reads a map file name") {
    const auto buffer = chunkWith(0xA300, {'t', 'e', 'x', 0x00});
    REQUIRE(buffer.size() == 10);
    auto chunk = TDSLoader::readChunk(buffer, 0);
    TDSLoader::MapTransform transform;
    TDSLoader::applyMapChunk(buffer, chunk, transform);
    REQUIRE(transform.name == "tex");
    // Naming a file does not touch the four numbers.
    REQUIRE(transform.repeatX == 1.0);
}

TEST_CASE("TDSLoader ignores a map chunk it does not know") {
    const auto buffer = chunkWith(0x9999, {1, 2});
    auto chunk = TDSLoader::readChunk(buffer, 0);
    TDSLoader::MapTransform transform;
    TDSLoader::applyMapChunk(buffer, chunk, transform);
    // Nothing changed, and the cursor did not move because the sub chunk was built elsewhere.
    REQUIRE(transform.name.empty());
    REQUIRE(transform.offsetX == 0.0);
}

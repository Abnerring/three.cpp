#include "loaders/MDDLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void putU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}
void putF32(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    static_assert(sizeof(float) == sizeof(bits), "a float must be four bytes");
    std::memcpy(&bits, &value, sizeof(bits));
    putU32(out, bits);
}
// A file of the given shape with the times 0.5, 1.0, ... and positions counting up.
std::vector<std::uint8_t> makeFile(std::uint32_t frames, std::uint32_t points) {
    std::vector<std::uint8_t> out;
    putU32(out, frames);
    putU32(out, points);
    for (std::uint32_t i = 0; i < frames; ++i) putF32(out, 0.5f * float(i + 1));
    for (std::uint32_t i = 0; i < frames; ++i) {
        for (std::uint32_t j = 0; j < points * 3; ++j) putF32(out, float(i * 100 + j));
    }
    return out;
}
}  // namespace

TEST_CASE("MDDLoader reads the two numbers a file begins with") {
    const auto file = makeFile(2, 3);
    const auto header = MDDLoader::header(file);
    REQUIRE(header.frames == 2);
    REQUIRE(header.points == 3);
    // A buffer too short for both has neither.
    const auto short_ = MDDLoader::header({1, 2, 3});
    REQUIRE(short_.frames == 0);
    REQUIRE(short_.points == 0);
}

TEST_CASE("MDDLoader sizes the sections") {
    const auto header = MDDLoader::Header{2, 3};
    // One time per frame and three numbers per point.
    REQUIRE(MDDLoader::timeCount(header) == 2);
    REQUIRE(MDDLoader::positionCount(header) == 9);
    // The header, the times and every frame's positions.
    REQUIRE(MDDLoader::byteSize(header) == 8 + 8 + 72);
    // A file built to that shape is exactly that long.
    REQUIRE(makeFile(2, 3).size() == MDDLoader::byteSize(header));
}

TEST_CASE("MDDLoader reads the times") {
    const auto file = makeFile(2, 3);
    const auto times = MDDLoader::times(file, MDDLoader::header(file));
    REQUIRE(times.size() == 2);
    REQUIRE(times[0] == 0.5);
    REQUIRE(times[1] == 1.0);
    // A buffer too short for the times gives none rather than reading past it.
    REQUIRE(MDDLoader::times({0, 0, 0, 0, 0, 0, 0, 0}, MDDLoader::Header{2, 3}).empty());
}

TEST_CASE("MDDLoader builds a square influence matrix") {
    // Two frames make two rows of two, with a one in the diagonal of each.
    const auto two = MDDLoader::influences(MDDLoader::Header{2, 0});
    REQUIRE(two.size() == 4);
    REQUIRE(two[0] == 1);
    REQUIRE(two[1] == 0);
    REQUIRE(two[2] == 0);
    REQUIRE(two[3] == 1);
    // Three frames make three rows of three.
    const auto three = MDDLoader::influences(MDDLoader::Header{3, 0});
    REQUIRE(three.size() == 9);
    REQUIRE(three[0] == 1);
    REQUIRE(three[4] == 1);
    REQUIRE(three[8] == 1);
    REQUIRE(three[1] == 0);
    REQUIRE(three[3] == 0);
    // Each row holds exactly one one, which is what makes a frame show one target.
    for (std::size_t row = 0; row < 3; ++row) {
        double sum = 0;
        for (std::size_t column = 0; column < 3; ++column) sum += three[row * 3 + column];
        REQUIRE(sum == 1.0);
    }
}

TEST_CASE("MDDLoader names the morph targets") {
    REQUIRE(MDDLoader::morphName(0) == "morph_0");
    REQUIRE(MDDLoader::morphName(1) == "morph_1");
    REQUIRE(MDDLoader::morphName(42) == "morph_42");
}

TEST_CASE("MDDLoader reads one frame of positions") {
    const auto file = makeFile(2, 3);
    const auto header = MDDLoader::header(file);
    // The first frame counts from zero.
    const auto first = MDDLoader::framePositions(file, header, 0);
    REQUIRE(first.size() == 9);
    REQUIRE(first[0] == 0.0);
    REQUIRE(first[8] == 8.0);
    // The second frame starts at a hundred.
    const auto second = MDDLoader::framePositions(file, header, 1);
    REQUIRE(second.size() == 9);
    REQUIRE(second[0] == 100.0);
    // A frame past the end gives nothing.
    REQUIRE(MDDLoader::framePositions(file, header, 2).empty());
}
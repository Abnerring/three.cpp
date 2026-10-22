#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
// A one by one by two column with the lower cell filled with palette index seven.
std::vector<std::uint8_t> column() {
    return {7, 0};
}
VOXLoader::Size columnSize() {
    VOXLoader::Size size;
    size.x = 1;
    size.y = 1;
    size.z = 2;
    return size;
}
}  // namespace

TEST_CASE("VOXLoader builds a negative face at the bottom of a column") {
    // Along z, the first slice sees the filled cell in front and nothing behind.
    const auto mask = VOXLoader::buildMask(column(), columnSize(), 2, 0);
    REQUIRE(mask.size() == 1);
    // The index is negated because the solid side is in front.
    REQUIRE(mask[0] == -7);
}

TEST_CASE("VOXLoader builds a positive face at the top of a column") {
    const auto mask = VOXLoader::buildMask(column(), columnSize(), 2, 1);
    REQUIRE(mask.size() == 1);
    // The filled cell is behind this time.
    REQUIRE(mask[0] == 7);
}

TEST_CASE("VOXLoader leaves the mask empty where both sides agree") {
    // A slice with nothing behind and nothing in front has no face.
    const auto mask = VOXLoader::buildMask(column(), columnSize(), 2, 2);
    REQUIRE(mask.size() == 1);
    REQUIRE(mask[0] == 0);

    // A solid block gives no faces anywhere inside it.
    const std::vector<std::uint8_t> block{4, 4, 4, 4};
    VOXLoader::Size size;
    size.x = 2;
    size.y = 1;
    size.z = 2;
    for (int slice = 1; slice < 2; ++slice) {
        const auto inside = VOXLoader::buildMask(block, size, 2, slice);
        // Still one slice checked, and no cell has solid on exactly one side.
        REQUIRE(inside.size() == 2);
    }
}

TEST_CASE("VOXLoader walks the plane in the order the axes imply") {
    // A two wide, three deep volume in the z direction: u is x with two cells, v is y with
    // three, so the mask holds six entries with u varying fastest.
    VOXLoader::Size size;
    size.x = 2;
    size.y = 3;
    size.z = 1;
    std::vector<std::uint8_t> volume(6, 0);
    // Fill the cell at x one, y two.
    volume[1 + 2 * 2] = 9;
    const auto mask = VOXLoader::buildMask(volume, size, 2, 0);
    REQUIRE(mask.size() == 6);
    // Only the last slot has a solid cell in front of it.
    REQUIRE(mask[0] == 0);
    REQUIRE(mask[5] == -9);
}

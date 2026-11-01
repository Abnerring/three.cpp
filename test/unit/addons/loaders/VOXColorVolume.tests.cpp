#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
VOXLoader::Voxel withColor(int x, int y, int z, int color) {
    VOXLoader::Voxel voxel;
    voxel.x = x;
    voxel.y = y;
    voxel.z = z;
    voxel.colorIndex = color;
    return voxel;
}
}  // namespace

TEST_CASE("VOXLoader stores the palette index per cell") {
    VOXLoader::Size size;
    size.x = 2;
    size.y = 2;
    size.z = 2;
    // One voxel at the last cell with palette index seven.
    const std::vector<VOXLoader::Voxel> voxels{withColor(1, 0, 1, 7)};
    const auto volume = VOXLoader::buildColorVolume(voxels, size);
    REQUIRE(volume.size() == 8);
    // The index is x plus y times two plus z times four.
    REQUIRE(volume[1 + 0 + 1 * 4] == 7);
    // Every other cell is empty.
    std::size_t filled = 0;
    for (const auto value : volume) if (value != 0) ++filled;
    REQUIRE(filled == 1);
}

TEST_CASE("VOXLoader differs from the occupancy grid") {
    VOXLoader::Size size;
    size.x = 1;
    size.y = 1;
    size.z = 1;
    const std::vector<VOXLoader::Voxel> voxels{withColor(0, 0, 0, 3)};
    // The texture path marks any occupied cell with 255, the mesher needs the real index.
    REQUIRE(VOXLoader::buildOccupancy(voxels, size)[0] == 255);
    REQUIRE(VOXLoader::buildColorVolume(voxels, size)[0] == 3);
}

TEST_CASE("VOXLoader ignores out of range voxels in the colour volume") {
    VOXLoader::Size size;
    size.x = 2;
    size.y = 2;
    size.z = 2;
    const std::vector<VOXLoader::Voxel> voxels{withColor(0, 0, 0, 1), withColor(9, 0, 0, 2)};
    const auto volume = VOXLoader::buildColorVolume(voxels, size);
    std::size_t filled = 0;
    for (const auto value : volume) if (value != 0) ++filled;
    REQUIRE(filled == 1);
}

TEST_CASE("VOXLoader returns nothing for an empty colour volume") {
    VOXLoader::Size size;
    REQUIRE(VOXLoader::buildColorVolume({withColor(0, 0, 0, 1)}, size).empty());
}

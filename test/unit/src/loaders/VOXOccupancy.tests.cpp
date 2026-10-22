#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
VOXLoader::Voxel at(int x, int y, int z) {
    VOXLoader::Voxel voxel;
    voxel.x = x;
    voxel.y = y;
    voxel.z = z;
    return voxel;
}
}  // namespace

TEST_CASE("VOXLoader marks occupied cells") {
    VOXLoader::Size size;
    size.x = 2;
    size.y = 3;
    size.z = 4;
    const std::vector<VOXLoader::Voxel> voxels{at(1, 2, 3)};
    const auto array = VOXLoader::buildOccupancy(voxels, size);
    // Two by three by four cells.
    REQUIRE(array.size() == 24);
    // The index runs x, then y across a row of two, then z across a plane of six.
    const std::size_t index = 1 + 2 * 2 + 3 * 6;
    REQUIRE(index == 23);
    REQUIRE(array[index] == 255);
    // Every other cell stays empty.
    std::size_t occupied = 0;
    for (const auto value : array) if (value != 0) ++occupied;
    REQUIRE(occupied == 1);
}

TEST_CASE("VOXLoader places two voxels independently") {
    VOXLoader::Size size;
    size.x = 2;
    size.y = 2;
    size.z = 2;
    const std::vector<VOXLoader::Voxel> voxels{at(0, 0, 0), at(1, 1, 1)};
    const auto array = VOXLoader::buildOccupancy(voxels, size);
    REQUIRE(array.size() == 8);
    REQUIRE(array[0] == 255);
    // The far corner is the last cell.
    REQUIRE(array[7] == 255);
}

TEST_CASE("VOXLoader ignores voxels outside the grid") {
    VOXLoader::Size size;
    size.x = 2;
    size.y = 2;
    size.z = 2;
    // One inside, one beyond the edge, one negative.
    const std::vector<VOXLoader::Voxel> voxels{at(0, 0, 0), at(5, 0, 0), at(-1, 0, 0)};
    const auto array = VOXLoader::buildOccupancy(voxels, size);
    std::size_t occupied = 0;
    for (const auto value : array) if (value != 0) ++occupied;
    REQUIRE(occupied == 1);
}

TEST_CASE("VOXLoader returns nothing for an empty grid") {
    VOXLoader::Size size;
    // A zero dimension means there is no texture to build.
    REQUIRE(VOXLoader::buildOccupancy({at(0, 0, 0)}, size).empty());
    size.x = 2;
    REQUIRE(VOXLoader::buildOccupancy({at(0, 0, 0)}, size).empty());
}

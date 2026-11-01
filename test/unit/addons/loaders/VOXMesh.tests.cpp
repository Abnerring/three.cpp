#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace {
VOXLoader::Size makeSize(int x, int y, int z) {
    VOXLoader::Size size;
    size.x = x;
    size.y = y;
    size.z = z;
    return size;
}
std::vector<std::array<std::uint8_t, 4>> palette() {
    // Entry one is pure red, entry two pure green.
    return {{{0, 0, 0, 0}}, {{255, 0, 0, 255}}, {{0, 255, 0, 255}}};
}
}  // namespace

TEST_CASE("VOXLoader meshes a single voxel into six faces") {
    VOXLoader::Voxel voxel;
    voxel.colorIndex = 1;
    const auto geometry = VOXLoader::buildMesh({voxel}, makeSize(1, 1, 1), palette());
    // One cube means six quads, so twenty four corners and thirty six indices.
    REQUIRE(geometry.positions.size() == 24 * 3);
    REQUIRE(geometry.indices.size() == 36);
    REQUIRE(geometry.colors.size() == 24 * 3);
}

TEST_CASE("VOXLoader shades every face with the voxel colour") {
    VOXLoader::Voxel voxel;
    voxel.colorIndex = 1;
    const auto geometry = VOXLoader::buildMesh({voxel}, makeSize(1, 1, 1), palette());
    // Pure red survives the conversion and the other channels vanish.
    REQUIRE(geometry.colors[0] == Approx(1.0).margin(1e-6));
    REQUIRE(geometry.colors[1] == Approx(0.0).margin(1e-6));
    REQUIRE(geometry.colors[2] == Approx(0.0).margin(1e-6));
}

TEST_CASE("VOXLoader uses the magnitude of the mask value for the colour") {
    VOXLoader::Voxel voxel;
    voxel.colorIndex = 2;
    const auto geometry = VOXLoader::buildMesh({voxel}, makeSize(1, 1, 1), palette());
    // Green is entry two, and every corner uses it.
    REQUIRE(geometry.colors[1] == Approx(1.0).margin(1e-6));
    REQUIRE(geometry.colors[0] == Approx(0.0).margin(1e-6));
}

TEST_CASE("VOXLoader returns nothing for an empty volume") {
    VOXLoader::Size size;
    const auto geometry = VOXLoader::buildMesh({}, size, palette());
    REQUIRE(geometry.positions.empty());
    REQUIRE(geometry.indices.empty());
    REQUIRE(geometry.colors.empty());
}

TEST_CASE("VOXLoader falls back to white for a missing palette entry") {
    VOXLoader::Voxel voxel;
    // Entry nine is past the end of the three entry palette.
    voxel.colorIndex = 9;
    const auto geometry = VOXLoader::buildMesh({voxel}, makeSize(1, 1, 1), palette());
    // White stays one in every channel after the conversion.
    REQUIRE(geometry.colors[0] == Approx(1.0).margin(1e-6));
    REQUIRE(geometry.colors[1] == Approx(1.0).margin(1e-6));
    REQUIRE(geometry.colors[2] == Approx(1.0).margin(1e-6));
}
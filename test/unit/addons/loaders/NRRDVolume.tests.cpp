#include "loaders/NRRDLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NRRDLoader takes the volume lengths from the sizes") {
    auto header = NRRDLoader::parseHeader("NRRD0005\ndimension: 3\nsizes: 2 3 4\n");
    const auto volume = NRRDLoader::buildVolume(header, {1, 2, 3, 4, 5, 6});
    REQUIRE(volume.xLength == 2);
    REQUIRE(volume.yLength == 3);
    REQUIRE(volume.zLength == 4);
    REQUIRE(volume.data.size() == 6);
}

TEST_CASE("NRRDLoader leaves a missing depth at one") {
    // A two dimensional file has only two sizes, so the depth defaults to one.
    auto header = NRRDLoader::parseHeader("NRRD0005\ndimension: 2\nsizes: 4 5\n");
    const auto volume = NRRDLoader::buildVolume(header, {});
    REQUIRE(volume.xLength == 4);
    REQUIRE(volume.yLength == 5);
    REQUIRE(volume.zLength == 1);
}

TEST_CASE("NRRDLoader leaves every missing length at one") {
    auto header = NRRDLoader::parseHeader("NRRD0005\nencoding: raw\n");
    const auto volume = NRRDLoader::buildVolume(header, {});
    // No sizes at all, so each axis is a single voxel rather than zero.
    REQUIRE(volume.xLength == 1);
    REQUIRE(volume.yLength == 1);
    REQUIRE(volume.zLength == 1);
}

TEST_CASE("NRRDLoader refuses a gzip body") {
    // Neither bzip nor gzip can be decoded without a decompressor, so both are reported.
    REQUIRE_THROWS_WITH(NRRDLoader::parseHeader("NRRD0005\nencoding: gzip\n"),
                        "THREE.NRRDLoader: Gzip is not supported");
}

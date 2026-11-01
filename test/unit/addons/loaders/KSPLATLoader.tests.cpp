#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

TEST_CASE("KSPLATLoader lays out the uncompressed level") {
    const auto layout = KSPLATLoader::compression(0);
    REQUIRE(layout.bytesPerCenter == 12);
    REQUIRE(layout.bytesPerScale == 12);
    REQUIRE(layout.bytesPerRotation == 16);
    REQUIRE(layout.bytesPerColor == 4);
    // Floats carry no implied range.
    REQUIRE(layout.scaleRange == 1);
    // The offsets are the widths before them added up.
    REQUIRE(layout.scaleOffset == layout.bytesPerCenter);
    REQUIRE(layout.rotationOffset == layout.scaleOffset + layout.bytesPerScale);
    REQUIRE(layout.colorOffset == layout.rotationOffset + layout.bytesPerRotation);
}

TEST_CASE("KSPLATLoader lays out the compressed levels") {
    for (const int level : {1, 2}) {
        const auto layout = KSPLATLoader::compression(level);
        REQUIRE(layout.bytesPerCenter == 6);
        REQUIRE(layout.bytesPerRotation == 8);
        REQUIRE(layout.scaleRange == 32767);
        // The offsets still line up with the widths.
        REQUIRE(layout.rotationOffset == layout.scaleOffset + layout.bytesPerScale);
        REQUIRE(layout.colorOffset == layout.rotationOffset + layout.bytesPerRotation);
    }
    // Only the harmonic width tells the two apart.
    REQUIRE(KSPLATLoader::compression(1).bytesPerHarmonicComponent == 2);
    REQUIRE(KSPLATLoader::compression(2).bytesPerHarmonicComponent == 1);
}

TEST_CASE("KSPLATLoader leaves an undefined level empty") {
    const auto layout = KSPLATLoader::compression(7);
    REQUIRE(layout.bytesPerCenter == 0);
    REQUIRE(layout.scaleRange == 1);
}

TEST_CASE("KSPLATLoader maps the harmonic components back into order") {
    // The first band lists its nine components in this order.
    const int expected[9] = {0, 3, 6, 1, 4, 7, 2, 5, 8};
    for (int i = 0; i < 9; ++i) REQUIRE(KSPLATLoader::harmonicIndex(1, i) == expected[i]);
    // The second band starts at nine.
    REQUIRE(KSPLATLoader::harmonicIndex(2, 0) == 9);
    REQUIRE(KSPLATLoader::harmonicIndex(2, 14) == 23);
    // The third runs to forty four.
    REQUIRE(KSPLATLoader::harmonicIndex(3, 0) == 24);
    REQUIRE(KSPLATLoader::harmonicIndex(3, 20) == 44);
}

TEST_CASE("KSPLATLoader rejects an out of range harmonic") {
    // Degree zero has no components, and neither does a negative degree.
    REQUIRE(KSPLATLoader::harmonicIndex(0, 0) == -1);
    REQUIRE(KSPLATLoader::harmonicIndex(-1, 0) == -1);
    // A component past the end of its band is rejected.
    REQUIRE(KSPLATLoader::harmonicIndex(1, 9) == -1);
    REQUIRE(KSPLATLoader::harmonicIndex(4, 0) == -1);
}

TEST_CASE("KSPLATLoader sizes one splat record") {
    // Uncompressed degree zero: twelve plus twelve plus sixteen plus four.
    REQUIRE(KSPLATLoader::splatStride(0, 0) == 44);
    // Degree one adds nine four byte components.
    REQUIRE(KSPLATLoader::splatStride(0, 1) == 44 + 36);
    // The compressed levels are far smaller.
    REQUIRE(KSPLATLoader::splatStride(1, 0) == 6 + 6 + 8 + 4);
    REQUIRE(KSPLATLoader::splatStride(2, 3) == 24 + 45);
}
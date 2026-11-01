#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

namespace {
KSPLATLoader::SectionHeader section(double blockSize, std::uint32_t range) {
    KSPLATLoader::SectionHeader header;
    header.bucketBlockSize = blockSize;
    header.compressionScaleRange = range;
    return header;
}
}  // namespace

TEST_CASE("KSPLATLoader computes the compression step") {
    // A block of four over a range that spans the whole of a sixteen bit field.
    const auto header = section(4.0, 32767);
    REQUIRE(KSPLATLoader::compressionScaleFactor(header) ==
            Approx(4.0 / 2.0 / 32767.0).epsilon(1e-15));
    // Doubling the block doubles the step.
    REQUIRE(KSPLATLoader::compressionScaleFactor(section(8.0, 32767)) ==
            Approx(2.0 * KSPLATLoader::compressionScaleFactor(header)).epsilon(1e-15));
}

TEST_CASE("KSPLATLoader guards a zero range") {
    // Without a range there is no step rather than a division by zero.
    REQUIRE(KSPLATLoader::compressionScaleFactor(section(4.0, 0)) == 0.0);
}

TEST_CASE("KSPLATLoader puts the middle of the range on the bucket") {
    const auto header = section(4.0, 32767);
    // The middle stored value lands exactly on the bucket coordinate.
    REQUIRE(KSPLATLoader::decodeCenter(32767, header, 10.0) == Approx(10.0).epsilon(1e-12));
}

TEST_CASE("KSPLATLoader spans half a block either side") {
    const auto header = section(4.0, 32767);
    // The lowest value is half a block below the bucket and the highest the same above.
    REQUIRE(KSPLATLoader::decodeCenter(0, header, 10.0) == Approx(8.0).epsilon(1e-9));
    REQUIRE(KSPLATLoader::decodeCenter(65534, header, 10.0) == Approx(12.0).epsilon(1e-9));
}

TEST_CASE("KSPLATLoader places each bucket at its own coordinate") {
    const auto header = section(4.0, 32767);
    // The same stored value in two buckets differs only by the bucket coordinate.
    REQUIRE(KSPLATLoader::decodeCenter(16383, header, 0.0) ==
            Approx(KSPLATLoader::decodeCenter(16383, header, 100.0) - 100.0).epsilon(1e-9));
}
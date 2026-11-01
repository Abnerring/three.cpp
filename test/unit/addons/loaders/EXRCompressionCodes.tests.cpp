#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("EXRLoader numbers the ten compressions") {
    // The numbers are the format's own and do not follow the alphabetical order of the names.
    REQUIRE(EXRLoader::kNoCompressionCode == 0);
    REQUIRE(EXRLoader::kRleCompressionCode == 1);
    REQUIRE(EXRLoader::kZipsCompressionCode == 2);
    REQUIRE(EXRLoader::kZipCompressionCode == 3);
    REQUIRE(EXRLoader::kPizCompressionCode == 4);
    REQUIRE(EXRLoader::kPxr24CompressionCode == 5);
    REQUIRE(EXRLoader::kB44CompressionCode == 6);
    REQUIRE(EXRLoader::kB44aCompressionCode == 7);
    REQUIRE(EXRLoader::kDwaaCompressionCode == 8);
    REQUIRE(EXRLoader::kDwabCompressionCode == 9);
}

TEST_CASE("EXRLoader names each number") {
    // Every number from nought to nine has a name, and no two share one.
    REQUIRE(EXRLoader::compressionNameOf(0) == "NO_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(1) == "RLE_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(2) == "ZIPS_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(3) == "ZIP_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(4) == "PIZ_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(5) == "PXR24_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(6) == "B44_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(7) == "B44A_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(8) == "DWAA_COMPRESSION");
    REQUIRE(EXRLoader::compressionNameOf(9) == "DWAB_COMPRESSION");
    // The two zips and the two B44s are told apart, which is where a mistake would hide.
    REQUIRE(EXRLoader::compressionNameOf(2) != EXRLoader::compressionNameOf(3));
    REQUIRE(EXRLoader::compressionNameOf(6) != EXRLoader::compressionNameOf(7));
    // A number outside the ten has no name rather than a made up one.
    REQUIRE(EXRLoader::compressionNameOf(10).empty());
    REQUIRE(EXRLoader::compressionNameOf(-1).empty());
}

TEST_CASE("EXRLoader reads the header flags") {
    // The first bit marks a tiled file and the fourth long names, the fifth deep data.
    REQUIRE_FALSE(EXRLoader::tiledFlag(0));
    REQUIRE(EXRLoader::tiledFlag(1));
    REQUIRE_FALSE(EXRLoader::hasLongNames(0));
    REQUIRE(EXRLoader::hasLongNames(4));
    REQUIRE_FALSE(EXRLoader::hasDeepData(0));
    REQUIRE(EXRLoader::hasDeepData(8));
    // The three do not overlap.
    REQUIRE(EXRLoader::tiledFlag(1));
    REQUIRE_FALSE(EXRLoader::hasDeepData(1));
    REQUIRE_FALSE(EXRLoader::hasLongNames(8));
}

TEST_CASE("EXRLoader refuses a flag it does not know") {
    // The low five bits are the ones the format uses, so a value inside them is fine.
    REQUIRE_FALSE(EXRLoader::unusedFlagSet(0));
    REQUIRE_FALSE(EXRLoader::unusedFlagSet(0x1f));
    // Anything above them is not.
    REQUIRE(EXRLoader::unusedFlagSet(0x20));
    REQUIRE(EXRLoader::unusedFlagSet(0x100));
}
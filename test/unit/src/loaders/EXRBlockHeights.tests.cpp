#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

TEST_CASE("EXRLoader gives each compression its block height") {
    // The three that work a line at a time take one scanline.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kNoCompressionCode) == 1);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kRleCompressionCode) == 1);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kZipsCompressionCode) == 1);
    // The sixteen line zips takes sixteen, and the two block compressions thirty two.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kZipCompressionCode) == 16);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kPizCompressionCode) == 32);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kB44CompressionCode) == 32);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kB44aCompressionCode) == 32);
    // The pixel rate takes sixteen.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kPxr24CompressionCode) == 16);
    // The two deep ones differ: thirty two and two hundred and fifty six.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kDwaaCompressionCode) == 32);
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kDwabCompressionCode) == 256);
}

TEST_CASE("EXRLoader tells the pairs apart by their block height") {
    // The two zips are the clearest case: one line against sixteen, which is the only thing
    // their names differ about.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kZipsCompressionCode) !=
            EXRLoader::blockHeightFor(EXRLoader::kZipCompressionCode));
    // The two deep ones differ by a factor of eight.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kDwabCompressionCode) ==
            8 * EXRLoader::blockHeightFor(EXRLoader::kDwaaCompressionCode));
    // The two block compressions do not differ at all.
    REQUIRE(EXRLoader::blockHeightFor(EXRLoader::kB44CompressionCode) ==
            EXRLoader::blockHeightFor(EXRLoader::kB44aCompressionCode));
}

TEST_CASE("EXRLoader covers nothing for a number it does not know") {
    REQUIRE(EXRLoader::blockHeightFor(10) == 0);
    REQUIRE(EXRLoader::blockHeightFor(-1) == 0);
}

TEST_CASE("EXRLoader names a compression it cannot read") {
    REQUIRE(EXRLoader::unsupportedCompressionMessage("PIZ_COMPRESSION") ==
            "THREE.EXRLoader: PIZ_COMPRESSION is unsupported");
    REQUIRE(EXRLoader::unsupportedCompressionMessage("X") !=
            EXRLoader::unsupportedCompressionMessage("Y"));
}
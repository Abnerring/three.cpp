#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("EXRLoader names the three deep compressions") {
    // The names are what a block says, not numbers.
    REQUIRE(std::string(EXRLoader::kNoCompression) == "NO_COMPRESSION");
    REQUIRE(std::string(EXRLoader::kRleCompression) == "RLE_COMPRESSION");
    REQUIRE(std::string(EXRLoader::kZipsCompression) == "ZIPS_COMPRESSION");
    // The single scanline zips and the multi scanline zip are different names, so a block that
    // says ZIP is not the same as one that says ZIPS.
    REQUIRE(std::string(EXRLoader::kZipsCompression) != "ZIP_COMPRESSION");
}

TEST_CASE("EXRLoader tells the three apart and refuses the rest") {
    REQUIRE(EXRLoader::compressionKind("NO_COMPRESSION") == EXRLoader::CompressionKind::None);
    REQUIRE(EXRLoader::compressionKind("RLE_COMPRESSION") ==
            EXRLoader::CompressionKind::RunLength);
    REQUIRE(EXRLoader::compressionKind("ZIPS_COMPRESSION") == EXRLoader::CompressionKind::Zip);
    // The multi scanline zip is not one of the three, so it is refused rather than guessed at.
    REQUIRE(EXRLoader::compressionKind("ZIP_COMPRESSION") ==
            EXRLoader::CompressionKind::Unsupported);
    REQUIRE(EXRLoader::compressionKind("PIZ_COMPRESSION") ==
            EXRLoader::CompressionKind::Unsupported);
    // Case matters, since the comparison is on the exact string.
    REQUIRE(EXRLoader::compressionKind("no_compression") ==
            EXRLoader::CompressionKind::Unsupported);
    REQUIRE(EXRLoader::compressionKind("") == EXRLoader::CompressionKind::Unsupported);
}

TEST_CASE("EXRLoader names the compression it turned away") {
    // The message carries the name it was given, so a reader can tell which block failed.
    REQUIRE(EXRLoader::unsupportedDeepMessage("PIZ_COMPRESSION") ==
            "THREE.EXRLoader: PIZ_COMPRESSION is unsupported for deep data");
    REQUIRE(EXRLoader::unsupportedDeepMessage("ZIP_COMPRESSION") ==
            "THREE.EXRLoader: ZIP_COMPRESSION is unsupported for deep data");
    // Two different names give two different messages.
    REQUIRE(EXRLoader::unsupportedDeepMessage("A") != EXRLoader::unsupportedDeepMessage("B"));
    // The word deep is in the message, which tells a reader this is the deep path and not the
    // ordinary one.
    REQUIRE(EXRLoader::unsupportedDeepMessage("X").find("deep") != std::string::npos);
}
#include "loaders/KTX2Loader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("KTX2Loader maps the primaries onto a colour space") {
    // The ordinary gamut takes the sRGB space.
    REQUIRE(KTX2Loader::colorSpaceFor("BT709") == "SRGBColorSpace");
    // The wide gamut gets a space of its own rather than being treated as ordinary, which is the
    // whole reason the container names its primaries.
    REQUIRE(KTX2Loader::colorSpaceFor("DISPLAY_P3") == "DisplayP3ColorSpace");
    REQUIRE(KTX2Loader::colorSpaceFor("DISPLAY_P3") != KTX2Loader::colorSpaceFor("BT709"));
    // A container that does not say claims no colour space at all rather than guessing one, so a
    // texture authored without the information is not silently treated as ordinary.
    REQUIRE(KTX2Loader::colorSpaceFor("UNSPECIFIED") == "NoColorSpace");
    REQUIRE(KTX2Loader::colorSpaceFor("UNSPECIFIED") != KTX2Loader::colorSpaceFor("BT709"));
    // Anything else is treated as unspecified rather than assumed.
    REQUIRE(KTX2Loader::colorSpaceFor("") == "NoColorSpace");
    REQUIRE(KTX2Loader::colorSpaceFor("SOMETHING") == "NoColorSpace");
    REQUIRE(std::string(KTX2Loader::kTypeName) == "KTX2Loader");
}

TEST_CASE("KTX2Loader reads two supercompression schemes") {
    // Stored as they are, or wrapped in a stream.
    REQUIRE(KTX2Loader::isReadableSupercompression("NONE"));
    REQUIRE(KTX2Loader::isReadableSupercompression("ZSTD"));
    // A third scheme is refused rather than read as if it were one of the two.
    REQUIRE_FALSE(KTX2Loader::isReadableSupercompression("BROTLI"));
    REQUIRE_FALSE(KTX2Loader::isReadableSupercompression(""));
    REQUIRE(KTX2Loader::isReadableSupercompression("NONE") !=
            KTX2Loader::isReadableSupercompression("BROTLI"));
    REQUIRE(std::string(KTX2Loader::kSupercompressionNone) == "NONE");
    REQUIRE(std::string(KTX2Loader::kSupercompressionZstd) == "ZSTD");
}

TEST_CASE("KTX2Loader counts its instances outside the class") {
    // One creation raises the count and one disposal lowers it, so the two cancel.
    REQUIRE(KTX2Loader::activeLoaderCount(true, false) == 1);
    REQUIRE(KTX2Loader::activeLoaderCount(false, true) == -1);
    REQUIRE(KTX2Loader::activeLoaderCount(true, true) == 0);
    REQUIRE(KTX2Loader::activeLoaderCount(false, false) == 0);
    // The count belongs to the module, so it is shared by every loader rather than per instance.
    REQUIRE(KTX2Loader::activeLoaderCount(true, false) != KTX2Loader::activeLoaderCount(false, false));
    // The flag the loader reads to know whether the values need dividing before use.
    REQUIRE(std::string(KTX2Loader::kFlagAlphaPremultiplied) == "ALPHA_PREMULTIPLIED");
    REQUIRE(std::string(KTX2Loader::kTransferSrgb) == "SRGB");
}
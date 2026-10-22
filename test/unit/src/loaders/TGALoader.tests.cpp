#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <string>

namespace {
TGALoader::Header makeHeader(int type, int width = 2, int height = 2) {
    TGALoader::Header header;
    header.imageType = type;
    header.width = width;
    header.height = height;
    header.pixelSize = 32;
    return header;
}
}  // namespace

TEST_CASE("TGALoader accepts the direct colour types") {
    // Uncompressed and RLE RGB both pass without a colour map.
    for (const int type : {TGALoader::kTypeRgb, TGALoader::kTypeRleRgb, TGALoader::kTypeGrey,
                           TGALoader::kTypeRleGrey}) {
        REQUIRE_NOTHROW(TGALoader::checkHeader(makeHeader(type)));
    }
}

TEST_CASE("TGALoader requires a colour map for indexed images") {
    auto header = makeHeader(TGALoader::kTypeIndexed);
    header.colormapType = 1;
    header.colormapSize = 24;
    header.colormapLength = 256;
    REQUIRE_NOTHROW(TGALoader::checkHeader(header));

    // One entry too many, a wrong entry width and a missing map are all rejected.
    auto tooLong = header;
    tooLong.colormapLength = 257;
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(tooLong),
                        "THREE.TGALoader: Invalid type colormap data for indexed type.");

    auto wrongSize = header;
    wrongSize.colormapSize = 32;
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(wrongSize),
                        "THREE.TGALoader: Invalid type colormap data for indexed type.");

    auto noMap = header;
    noMap.colormapType = 0;
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(noMap),
                        "THREE.TGALoader: Invalid type colormap data for indexed type.");
}

TEST_CASE("TGALoader rejects a colour map on a direct colour type") {
    auto header = makeHeader(TGALoader::kTypeRgb);
    header.colormapType = 1;
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(header),
                        "THREE.TGALoader: Invalid type colormap data for colormap type.");
}

TEST_CASE("TGALoader rejects a no data image and an unknown type") {
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(makeHeader(TGALoader::kTypeNoData)),
                        "THREE.TGALoader: No data.");
    // The message names the type that was found.
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(makeHeader(99)), "THREE.TGALoader: Invalid type 99");
}

TEST_CASE("TGALoader rejects a non positive size") {
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(makeHeader(TGALoader::kTypeRgb, 0, 2)),
                        "THREE.TGALoader: Invalid image size.");
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(makeHeader(TGALoader::kTypeRgb, 2, -1)),
                        "THREE.TGALoader: Invalid image size.");
}

TEST_CASE("TGALoader knows which types are run length encoded") {
    REQUIRE(TGALoader::isRle(TGALoader::kTypeRleIndexed));
    REQUIRE(TGALoader::isRle(TGALoader::kTypeRleRgb));
    REQUIRE(TGALoader::isRle(TGALoader::kTypeRleGrey));
    REQUIRE_FALSE(TGALoader::isRle(TGALoader::kTypeRgb));
    REQUIRE_FALSE(TGALoader::isRle(TGALoader::kTypeIndexed));
}

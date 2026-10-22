#include "loaders/MTLLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("MTLLoader texture params default to identity for a bare url") {
    const auto params = MTLLoader::getTextureParams("textures/wall.png");
    REQUIRE(params.url == "textures/wall.png");
    REQUIRE(params.scaleU == 1.0);
    REQUIRE(params.scaleV == 1.0);
    REQUIRE(params.offsetU == 0.0);
    REQUIRE(params.offsetV == 0.0);
    REQUIRE_FALSE(params.bumpScale.has_value());
    REQUIRE_FALSE(params.displacementBias.has_value());
    REQUIRE_FALSE(params.displacementScale.has_value());
}

TEST_CASE("MTLLoader texture params lift the bump scale out of the line") {
    const auto params = MTLLoader::getTextureParams("wall.png -bm 2.5");
    REQUIRE(params.url == "wall.png");
    REQUIRE(params.bumpScale.has_value());
    REQUIRE(params.bumpScale.value() == 2.5);
}

TEST_CASE("MTLLoader texture params read repeat and offset when each is alone") {
    const auto scaled = MTLLoader::getTextureParams("wall.png -s 2 3");
    REQUIRE(scaled.url == "wall.png");
    REQUIRE(scaled.scaleU == 2.0);
    REQUIRE(scaled.scaleV == 3.0);

    const auto offset = MTLLoader::getTextureParams("wall.png -o 0.5 0.25");
    REQUIRE(offset.url == "wall.png");
    REQUIRE(offset.offsetU == 0.5);
    REQUIRE(offset.offsetV == 0.25);
}

TEST_CASE("MTLLoader texture params reproduce the upstream token loss") {
    // Upstream splices four tokens for -s, so when -o follows it the option marker and the
    // first offset value are swallowed, and the remaining numbers end up in the url. This
    // reproduces that exactly rather than fixing it.
    const auto params = MTLLoader::getTextureParams("wall.png -s 2 3 -o 0.5 0.25");
    REQUIRE(params.scaleU == 2.0);
    REQUIRE(params.scaleV == 3.0);
    REQUIRE(params.url == "wall.png 0.5 0.25");
    // The offset option was consumed before it could be read.
    REQUIRE(params.offsetU == 0.0);
    REQUIRE(params.offsetV == 0.0);
}

TEST_CASE("MTLLoader texture params read the displacement pair") {
    const auto params = MTLLoader::getTextureParams("wall.png -mm 0.1 2");
    REQUIRE(params.url == "wall.png");
    REQUIRE(params.displacementBias.has_value());
    REQUIRE(params.displacementBias.value() == Approx(0.1).epsilon(1e-12));
    REQUIRE(params.displacementScale.has_value());
    REQUIRE(params.displacementScale.value() == 2.0);
}

TEST_CASE("MTLLoader texture params drop four tokens for scale and offset") {
    // Upstream erases four tokens for -s and -o even though only two values are read, so a
    // trailing word next to the option is swallowed rather than kept in the url.
    const auto params = MTLLoader::getTextureParams("wall.png -s 2 2 trailing");
    REQUIRE(params.url == "wall.png");
    REQUIRE(params.scaleU == 2.0);
    REQUIRE(params.scaleV == 2.0);
}

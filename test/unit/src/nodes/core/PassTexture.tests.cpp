#include "nodes/core/PassTexture.h"

#include "catch.hpp"

#include <string>

TEST_CASE("PassTexture may be cached, unlike the rest of the layer") {
    // Every other node whose caching rule has been taken refuses outright; this one allows it,
    // so the refusal is a choice per module rather than a rule of the layer.
    REQUIRE(PassTexture::isCacheable());
    REQUIRE(PassTexture::isCacheable() != false);
    // The matrix is not updated, since the texture comes from a pass that has already written it.
    REQUIRE_FALSE(PassTexture::updatesMatrix);
    REQUIRE(std::string(PassTexture::kTypeName) == "PassTextureNode");
}

TEST_CASE("PassTexture's multiple kind holds two textures") {
    // The plain node holds one and the multiple kind two, the second being the one before for
    // the previous texture API.
    REQUIRE(PassTexture::textureCount(false) == 1);
    REQUIRE(PassTexture::textureCount(true) == 2);
    REQUIRE(PassTexture::kMultipleTextureCount == 2);
    // The two answers differ, which is the whole reason the second class exists.
    REQUIRE(PassTexture::textureCount(true) != PassTexture::textureCount(false));
    REQUIRE(std::string(PassTexture::kMultipleTypeName) == "PassMultipleTextureNode");
}

TEST_CASE("PassTexture's clone shares rather than copies") {
    // A clone is built from the same pass and the same value, so the two views read one texture.
    REQUIRE(PassTexture::cloneSharesTexture());
    // It is therefore not a deep copy, which is what a caller might assume from the name.
    REQUIRE(PassTexture::cloneSharesTexture() != false);
}
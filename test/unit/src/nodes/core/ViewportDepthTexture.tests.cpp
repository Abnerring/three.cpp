#include "nodes/core/ViewportDepthTexture.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ViewportDepthTexture makes one texture for every default caller") {
    ViewportDepthTexture::reset();
    // The first default caller makes the shared texture.
    const auto first = ViewportDepthTexture::textureId(false, 0);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
    // The second reuses it rather than making another, which is the rule this module exists for.
    const auto second = ViewportDepthTexture::textureId(false, 0);
    REQUIRE(second == first);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
    // A third is the same again.
    REQUIRE(ViewportDepthTexture::textureId(false, 0) == first);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
    // The count of callers rises even though only one texture was made.
    REQUIRE(ViewportDepthTexture::nodesServed() == 3);
    REQUIRE(std::string(ViewportDepthTexture::kTypeName) == "ViewportDepthTextureNode");
}

TEST_CASE("ViewportDepthTexture uses a texture that was given as it stands") {
    ViewportDepthTexture::reset();
    // A caller with its own texture gets that one, not the shared one.
    REQUIRE(ViewportDepthTexture::textureId(true, 77) == 77);
    // And it did not cause a shared texture to be made.
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 0);
    // A default caller afterwards does make one, and it is not the given texture.
    const auto shared = ViewportDepthTexture::textureId(false, 0);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
    REQUIRE(shared != 77);
}

TEST_CASE("ViewportDepthTexture serves every caller from the same shared texture") {
    ViewportDepthTexture::reset();
    const auto a = ViewportDepthTexture::textureId(false, 0);
    const auto b = ViewportDepthTexture::textureId(true, 5);
    const auto c = ViewportDepthTexture::textureId(false, 0);
    // The two default callers agree with each other and differ from the one with its own.
    REQUIRE(a == c);
    REQUIRE(a != b);
    REQUIRE(b == 5);
    // Three callers were served and one texture was made.
    REQUIRE(ViewportDepthTexture::nodesServed() == 3);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
}

TEST_CASE("ViewportDepthTexture starts clean after a reset") {
    ViewportDepthTexture::reset();
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 0);
    REQUIRE(ViewportDepthTexture::nodesServed() == 0);
    // The first caller after a reset makes a texture again, which shows the reset really
    // forgets it rather than merely hiding the count.
    const auto made = ViewportDepthTexture::textureId(false, 0);
    REQUIRE(ViewportDepthTexture::defaultTextureCount() == 1);
    REQUIRE(made != 0);
}
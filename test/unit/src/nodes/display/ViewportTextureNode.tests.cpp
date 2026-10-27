#include "nodes/display/ViewportTextureNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ViewportTexture's flag does not name its class") {
    // The flag says the node is an output texture, not a viewport one, so a test written
    // against the class name would fail.
    REQUIRE(std::string(ViewportTexture::kFlagName) == "isOutputTextureNode");
    REQUIRE(std::string(ViewportTexture::kFlagName) != "isViewportTextureNode");
    REQUIRE(std::string(ViewportTexture::kTypeName) == "ViewportTextureNode");
    // The two names differ, which is the point.
    REQUIRE(std::string(ViewportTexture::kFlagName) != std::string(ViewportTexture::kTypeName));
}

TEST_CASE("ViewportTexture sets a mipmapped filter and forbids mipmaps") {
    // The filter it sets reads mipmaps.
    REQUIRE(std::string(ViewportTexture::kMinFilter) == "LinearMipmapLinearFilter");
    // And yet mipmap generation is off, so the two settings point in opposite directions.
    REQUIRE_FALSE(ViewportTexture::generatesMipmaps);
    REQUIRE_FALSE(ViewportTexture::filterAndMipmapsAgree());
    // The filter is the minimum one rather than the magnification one, which is where a
    // transcription could put it.
    REQUIRE(std::string(ViewportTexture::kMinFilter) != "LinearMipmapLinearFilter " );
    REQUIRE(std::string(ViewportTexture::kMinFilter).find("Linear") == 0);
}

TEST_CASE("ViewportTexture refreshes before the render") {
    // The contents of the bound framebuffer are read at the start of a render, so the refresh is
    // a before one rather than an after one.
    REQUIRE(std::string(ViewportTexture::kUpdateBeforeType) == "RENDER");
    REQUIRE(std::string(ViewportTexture::kUpdateBeforeType) != "FRAME");
}

TEST_CASE("ViewportTexture makes a texture only when none was given") {
    // Giving one suppresses the default, and not giving one produces it.
    REQUIRE(ViewportTexture::makesDefaultTexture(false));
    REQUIRE_FALSE(ViewportTexture::makesDefaultTexture(true));
    // One texture per render target, which is what keeps two targets apart.
    REQUIRE(ViewportTexture::cachesPerRenderTarget);
}
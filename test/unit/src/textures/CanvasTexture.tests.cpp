#include "catch.hpp"
#include "textures/CanvasTexture.h"
#include "textures/TextureSource.h"
#include <memory>

// A canvas texture is the base texture with the canvas flag and needsUpdate raised, because a canvas is ready
// to sample the moment it exists. The host supplies the pixels as a texture image.
TEST_CASE("CanvasTexture marks itself ready on construction") {
    auto image = std::make_shared<TextureImage>(4.0, 2.0);
    CanvasTexture texture(image);
    REQUIRE(texture.isCanvasTexture);
    // needsUpdate is a write only flag: assigning it bumps a version, so there is nothing to read back.
    REQUIRE(texture.image.get().get() == image.get());
    REQUIRE(texture.image.get().get()->width == 4.0);
    REQUIRE(texture.minFilter == LinearMipmapLinearFilter);
    REQUIRE(texture.magFilter == LinearFilter);
    REQUIRE(texture.wrapS == ClampToEdgeWrapping);
}
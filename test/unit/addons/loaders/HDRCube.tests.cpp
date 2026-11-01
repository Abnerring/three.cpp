#include "loaders/HDRCubeTextureLoader.h"

#include "catch.hpp"

TEST_CASE("HDRCubeTextureLoader configures a half float cube texture") {
    HDRCubeTextureLoader loader;
    // Half float is the default upstream chooses for the cube variant.
    REQUIRE(loader.type() == HDRLoader::kHalfFloatType);
    const auto texture = loader.createTexture();
    REQUIRE(texture != nullptr);
    REQUIRE(texture->type == HDRLoader::kHalfFloatType);
    // Both float cases pin the colour space and filtering to linear with no mipmaps.
    REQUIRE(texture->colorSpace == LinearSRGBColorSpace);
    REQUIRE(texture->minFilter == LinearFilter);
    REQUIRE(texture->magFilter == LinearFilter);
    REQUIRE_FALSE(texture->generateMipmaps);
}

TEST_CASE("HDRCubeTextureLoader forwards the data type to its HDR loader") {
    HDRCubeTextureLoader loader;
    loader.setDataType(HDRLoader::kFloatType);
    REQUIRE(loader.type() == HDRLoader::kFloatType);
    // The owned parser agrees, which is what upstream's setDataType does by hand.
    REQUIRE(loader.hdrLoader().dataType() == HDRLoader::kFloatType);
    // The setter is chainable.
    REQUIRE(&loader.setDataType(HDRLoader::kHalfFloatType) == &loader);
}

TEST_CASE("HDRCubeTextureLoader leaves other types untouched") {
    HDRCubeTextureLoader loader;
    // An unsigned byte type matches neither case of the switch.
    loader.setDataType(UnsignedByteType);
    const auto texture = loader.createTexture();
    REQUIRE(texture->type == UnsignedByteType);
    // The four assignments did not run, so the defaults survive.
    REQUIRE(texture->generateMipmaps);
    REQUIRE(texture->colorSpace != LinearSRGBColorSpace);
}

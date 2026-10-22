#include "catch.hpp"
#include "core/RenderTarget.h"
#include "constants.h"
#include "extras/PMREMGenerator.h"
#include "textures/Texture.h"
#include "textures/DataTexture.h"
#include "textures/CubeTexture.h"
#include <cmath>

TEST_CASE("PMREMGenerator derives cube size the way upstream does") {
    PMREMGenerator generator;
    REQUIRE(generator.cubeSize() == 0);
    generator._setSize(100);   // below the floor, upsampled to the minimum
    REQUIRE(generator.cubeSize() == 256);
    generator._setSize(500);   // rounded down to a power of two
    REQUIRE(generator.cubeSize() == 256);
    generator._setSize(1024);
    REQUIRE(generator.cubeSize() == 1024);
    generator._setSize(256);
    REQUIRE(generator.cubeSize() == 256);
}

TEST_CASE("PMREMGenerator lays out one prefiltered mip per lod") {
    PMREMGenerator generator;
    generator._setSize(1024);
    auto target = generator.allocateTarget();
    REQUIRE(target != nullptr);
    REQUIRE(target->width == 1024);
    REQUIRE(target->height == 1024);
    const auto& lods = generator.lodSizes();
    // maxLod = log2(1024) - LOD_MIN = 7, so eight levels halving from the top.
    REQUIRE(lods.size() == 8);
    for (std::size_t i = 0; i < lods.size(); ++i)
        REQUIRE(lods[i] == static_cast<std::uint32_t>(1024 >> i));
    auto texture = target->texture.get();
    REQUIRE(texture->name == "PMREM");
    REQUIRE(texture->isPMREMTexture);
    REQUIRE(texture->mipmaps->size() == lods.size());
    for (std::size_t i = 0; i < lods.size(); ++i) {
        REQUIRE((*texture->mipmaps)[i]->width == lods[i]);
        REQUIRE((*texture->mipmaps)[i]->height == lods[i]);
    }
    REQUIRE(lods.back() == static_cast<std::uint32_t>(std::pow(2.0, PMREMGenerator::LOD_MIN)));
}

TEST_CASE("PMREMGenerator reports the shader passes it cannot run") {
    PMREMGenerator generator;
    REQUIRE_THROWS_AS(generator.fromScene(JSValue::object()), std::logic_error);
    REQUIRE_THROWS_AS(generator.fromEquirectangular(JSValue::object()), std::logic_error);
    REQUIRE_THROWS_AS(generator.fromCubemap(JSValue::object()), std::logic_error);
    REQUIRE_THROWS_AS(generator.compileCubemapShader(), std::logic_error);
    REQUIRE_THROWS_AS(generator.compileEquirectangularShader(), std::logic_error);
    generator.dispose();
    REQUIRE(generator.cubeSize() == 0);
}
TEST_CASE("PMREMGenerator derives the size from fromScene options before rendering") {
    PMREMGenerator generator;
    // Default size is 256, and the lod chain stops at LOD_MIN.
    REQUIRE_THROWS_AS(generator.fromScene(JSValue::object()), std::logic_error);
    REQUIRE(generator.cubeSize() == 256);
    REQUIRE(generator.lodSizes().size() == 6);
    REQUIRE(generator.lodSizes().back() == 8);

    auto options = JSValue::object();
    options.set("size", 512);
    REQUIRE_THROWS_AS(generator.fromScene(JSValue::object(), 0, 0.1, 100, options), std::logic_error);
    REQUIRE(generator.cubeSize() == 512);
    REQUIRE(generator.lodSizes().size() == 7);
    REQUIRE(generator.lodSizes().front() == 512);
    REQUIRE(generator.lodSizes().back() == 8);

    auto small = JSValue::object();
    small.set("size", 64);   // below the 256 floor, upsampled
    REQUIRE_THROWS_AS(generator.fromScene(JSValue::object(), 0, 0.1, 100, small), std::logic_error);
    REQUIRE(generator.cubeSize() == 256);
}
TEST_CASE("PMREMGenerator caches one source target per size and depth need") {
    PMREMGenerator generator;
    generator._setSize(512);
    auto plain = generator._getSourceTarget();
    REQUIRE(plain != nullptr);
    REQUIRE(plain->width == 512);
    REQUIRE(plain->depthBuffer == false);
    auto texture = plain->texture.get();
    REQUIRE(texture->generateMipmaps);
    REQUIRE(texture->minFilter == LinearMipmapLinearFilter);
    REQUIRE(texture->type == HalfFloatType);
    REQUIRE(texture->colorSpace == LinearSRGBColorSpace);
    // The same request reuses the same target.
    REQUIRE(generator._getSourceTarget() == plain);
    // Asking for a depth buffer replaces it.
    auto withDepth = generator._getSourceTarget(true);
    REQUIRE(withDepth != plain);
    REQUIRE(withDepth->depthBuffer == true);
    // Upstream asymmetry: going back to no depth keeps the depth capable target.
    REQUIRE(generator._getSourceTarget(false) == withDepth);
    // A size change replaces it again.
    generator._setSize(256);
    auto resized = generator._getSourceTarget();
    REQUIRE(resized != withDepth);
    REQUIRE(resized->width == 256);
}
TEST_CASE("PMREMGenerator sizes a cube target from the source texture") {
    PMREMGenerator generator;
    // Cubemap: the face width decides, rounded down and floored.
    {
        TextureImage::Faces faces;
        for (int i = 0; i < 6; ++i) {
            auto image = std::make_shared<TextureImage>();
            image->width = 1024;
            image->height = 1024;
            faces.push_back(std::move(image));
        }
        CubeTexture cube(TextureImage::array(faces));
        REQUIRE_THROWS_AS(generator._fromTexture(cube), std::logic_error);
        REQUIRE(generator.cubeSize() == 1024);
    }
    // An empty cubemap falls back to MIN_SIZE.
    {
        CubeTexture empty(TextureImage::array(TextureImage::Faces{}));
        REQUIRE_THROWS_AS(generator._fromTexture(empty), std::logic_error);
        REQUIRE(generator.cubeSize() == PMREMGenerator::MIN_SIZE);
    }
    // Equirectangular: a quarter of the width.
    {
        DataTexture wide(Uint8Array(std::vector<std::uint8_t>(4096 * 2048 * 4, 0)), 4096, 2048);
        REQUIRE_THROWS_AS(generator._fromTexture(wide), std::logic_error);
        REQUIRE(generator.cubeSize() == 1024);
    }
    // A width that is not a multiple of four still halves twice and floors to the minimum.
    {
        DataTexture narrow(Uint8Array(std::vector<std::uint8_t>(300 * 150 * 4, 0)), 300, 150);
        REQUIRE_THROWS_AS(generator._fromTexture(narrow), std::logic_error);
        REQUIRE(generator.cubeSize() == PMREMGenerator::MIN_SIZE);
    }
}
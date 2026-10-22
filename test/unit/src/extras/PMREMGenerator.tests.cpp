#include "catch.hpp"
#include "core/RenderTarget.h"
#include "extras/PMREMGenerator.h"
#include "textures/Texture.h"
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
#include "loaders/MTLLoader.h"
#include "materials/MeshPhongMaterial.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kMtl =
    "newmtl Red\n"
    "Kd 1 0 0\n"
    "Ks 0 1 0\n"
    "Ns 250\n"
    "d 0.25\n"
    "newmtl Blue\n"
    "Kd 0 0 1\n";
}

TEST_CASE("MTLLoader builds one material per record") {
    MTLLoader loader;
    const auto converted = MTLLoader::convert(loader.parse(kMtl), MTLLoader::MaterialOptions{});
    const auto materials = loader.getAsArray(converted, MTLLoader::MaterialOptions{});
    REQUIRE(materials.size() == 2);
    // Map ordering puts Blue before Red.
    REQUIRE(materials[0]->name == "Blue");
    REQUIRE(materials[1]->name == "Red");
    REQUIRE(loader.getIndex("Blue") == 0);
    REQUIRE(loader.getIndex("Red") == 1);
    REQUIRE(loader.getIndex("Missing") == -1);
}

TEST_CASE("MTLLoader carries the parsed values onto the material") {
    MTLLoader loader;
    const auto converted = MTLLoader::convert(loader.parse(kMtl), MTLLoader::MaterialOptions{});
    loader.getAsArray(converted, MTLLoader::MaterialOptions{});
    const auto red = loader.create("Red", converted, MTLLoader::MaterialOptions{});
    REQUIRE(red != nullptr);
    REQUIRE(red->shininess == 250.0);
    // d of 0.25 sets both the opacity and the transparent flag.
    REQUIRE(red->opacity == Approx(0.25).epsilon(1e-12));
    REQUIRE(red->transparent);
    // The specular colour came from Ks and was converted out of sRGB.
    REQUIRE(red->specular.g > 0.0);
    // Colour conversion means the raw one is not what lands on the material.
    REQUIRE(red->color.r > 0.0);
}

TEST_CASE("MTLLoader reuses a material it already built") {
    MTLLoader loader;
    const auto converted = MTLLoader::convert(loader.parse(kMtl), MTLLoader::MaterialOptions{});
    const auto first = loader.create("Red", converted, MTLLoader::MaterialOptions{});
    const auto second = loader.create("Red", converted, MTLLoader::MaterialOptions{});
    // The second call must hand back the same instance, not a rebuilt one.
    REQUIRE(first.get() == second.get());
    REQUIRE(loader.create("Missing", converted, MTLLoader::MaterialOptions{}) == nullptr);
}

TEST_CASE("MTLLoader applies the side option to every material") {
    MTLLoader loader;
    const auto converted = MTLLoader::convert(loader.parse(kMtl), MTLLoader::MaterialOptions{});
    MTLLoader::MaterialOptions options;
    options.side = DoubleSide;
    const auto red = loader.create("Red", converted, options);
    REQUIRE(red->side == DoubleSide);
}

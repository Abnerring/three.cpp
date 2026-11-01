#include "loaders/LUTCubeLoader.h"

#include "catch.hpp"

#include <stdexcept>
#include <string>

namespace {
const char* kCube =
    "TITLE \"Test LUT\"\n"
    "LUT_3D_SIZE 2\n"
    "DOMAIN_MIN 0.0 0.0 0.0\n"
    "DOMAIN_MAX 1.0 1.0 1.0\n"
    "0.0 0.0 0.0\n"
    "1.0 0.0 0.0\n"
    "0.0 1.0 0.0\n"
    "1.0 1.0 0.0\n"
    "0.0 0.0 1.0\n"
    "1.0 0.0 1.0\n"
    "0.0 1.0 1.0\n"
    "1.0 1.0 1.0\n";
}

TEST_CASE("LUTCubeLoader parses title, size and domain") {
    LUTCubeLoader loader;
    const auto result = loader.parse(kCube);
    REQUIRE(result.title.has_value());
    REQUIRE(result.title.value() == "Test LUT");
    REQUIRE(result.size == 2);
    REQUIRE(result.domainMin.x == 0);
    REQUIRE(result.domainMin.y == 0);
    REQUIRE(result.domainMin.z == 0);
    REQUIRE(result.domainMax.x == 1);
    REQUIRE(result.domainMax.y == 1);
    REQUIRE(result.domainMax.z == 1);
}

TEST_CASE("LUTCubeLoader scales data points to bytes and writes the alpha channel") {
    LUTCubeLoader loader;
    const auto result = loader.parse(kCube);
    REQUIRE(result.texture3D != nullptr);
    // A 2x2x2 LUT has eight RGBA entries.
    const auto& image = *result.texture3D->image.get();
    REQUIRE(image.width == 2);
    REQUIRE(image.height == 2);
    REQUIRE(image.depth == 2);

    const auto& bytes = std::get<Uint8Array>(image.data);
    REQUIRE(bytes.size() == 2 * 2 * 2 * 4);
    // The first point is black with a full alpha written from the scale.
    REQUIRE(bytes[0] == 0);
    REQUIRE(bytes[1] == 0);
    REQUIRE(bytes[2] == 0);
    REQUIRE(bytes[3] == 255);
    // The second point is pure red.
    REQUIRE(bytes[4] == 255);
    REQUIRE(bytes[5] == 0);
    REQUIRE(bytes[6] == 0);
    REQUIRE(bytes[7] == 255);
    // The last point is white.
    REQUIRE(bytes[28] == 255);
    REQUIRE(bytes[29] == 255);
    REQUIRE(bytes[30] == 255);
    REQUIRE(bytes[31] == 255);
}

TEST_CASE("LUTCubeLoader reports the upstream errors") {
    LUTCubeLoader loader;
    REQUIRE_THROWS_WITH(loader.parse("TITLE \"x\"\n"),
                        "THREE.LUTCubeLoader: Missing LUT_3D_SIZE information");
    REQUIRE_THROWS_WITH(loader.parse("LUT_3D_SIZE 2\nDOMAIN_MIN 1 1 1\nDOMAIN_MAX 0 0 0\n"),
                        "THREE.LUTCubeLoader: Invalid input domain");
}

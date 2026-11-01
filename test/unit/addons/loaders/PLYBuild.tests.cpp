#include "loaders/PLYLoader.h"

#include "core/BufferGeometry.h"

#include "catch.hpp"

TEST_CASE("PLYLoader builds a geometry with an index and its attributes") {
    PLYLoader::Buffer buffer;
    buffer.vertices = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    buffer.normals = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    buffer.uvs = {0, 0, 1, 0, 0, 1};
    buffer.colors = {255, 0, 0, 0, 255, 0, 0, 0, 255};
    buffer.indices = {0, 1, 2};

    const auto geometry = PLYLoader::buildGeometry(buffer);
    REQUIRE(geometry != nullptr);
    REQUIRE(geometry->getIndex() != nullptr);
    REQUIRE(geometry->getAttribute("position") != nullptr);
    REQUIRE(geometry->getAttribute("normal") != nullptr);
    REQUIRE(geometry->getAttribute("uv") != nullptr);
    REQUIRE(geometry->getAttribute("color") != nullptr);
}

TEST_CASE("PLYLoader leaves out attributes the file did not carry") {
    PLYLoader::Buffer buffer;
    buffer.vertices = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    // No normals, no uvs, no colours and no index.
    const auto geometry = PLYLoader::buildGeometry(buffer);
    REQUIRE(geometry->getAttribute("position") != nullptr);
    // Upstream does not compute missing normals, so this stays absent.
    REQUIRE(geometry->getAttribute("normal") == nullptr);
    REQUIRE(geometry->getAttribute("uv") == nullptr);
    REQUIRE(geometry->getAttribute("color") == nullptr);
    REQUIRE(geometry->getIndex() == nullptr);
}

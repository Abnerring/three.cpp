#include "loaders/PCDLoader.h"

#include "core/BufferGeometry.h"

#include "catch.hpp"

TEST_CASE("PCDLoader builds a geometry from the decoded streams") {
    PCDLoader::Points points;
    points.positions = {0, 0, 0, 1, 1, 1};
    points.colors = {1, 0, 0, 0, 1, 0};
    const auto geometry = PCDLoader::buildGeometry(points);
    REQUIRE(geometry != nullptr);
    REQUIRE(geometry->getAttribute("position") != nullptr);
    REQUIRE(geometry->getAttribute("color") != nullptr);
    // No normals were decoded, so no normal attribute is created.
    REQUIRE(geometry->getAttribute("normal") == nullptr);
    REQUIRE(geometry->getAttribute("intensity") == nullptr);
}

TEST_CASE("PCDLoader adds the scalar streams as one component attributes") {
    PCDLoader::Points points;
    points.positions = {0, 0, 0};
    points.normals = {0, 1, 0};
    points.intensity = {42};
    points.label = {7};
    const auto geometry = PCDLoader::buildGeometry(points);
    REQUIRE(geometry->getAttribute("normal") != nullptr);
    REQUIRE(geometry->getAttribute("intensity") != nullptr);
    REQUIRE(geometry->getAttribute("label") != nullptr);
}

TEST_CASE("PCDLoader builds an empty geometry from nothing") {
    const PCDLoader::Points empty;
    const auto geometry = PCDLoader::buildGeometry(empty);
    REQUIRE(geometry != nullptr);
    // Every stream was empty, so no attribute exists.
    REQUIRE(geometry->getAttribute("position") == nullptr);
    REQUIRE(geometry->getAttribute("color") == nullptr);
}

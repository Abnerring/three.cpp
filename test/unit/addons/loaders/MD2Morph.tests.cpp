#include "loaders/MD2Loader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
MD2Loader::Frame makeFrame(const std::string& name, double base) {
    MD2Loader::Frame frame;
    frame.header.name = name;
    for (int i = 0; i < 3; ++i) {
        MD2Loader::Vertex vertex;
        vertex.position[0] = base + i;
        vertex.position[1] = base + i + 10;
        vertex.position[2] = base + i + 20;
        vertex.normal[0] = 0;
        vertex.normal[1] = 0;
        vertex.normal[2] = 1;
        frame.vertices.push_back(vertex);
    }
    return frame;
}
}  // namespace

TEST_CASE("MD2Loader builds one morph target per frame") {
    std::vector<MD2Loader::Frame> frames{makeFrame("a", 0), makeFrame("b", 100)};
    MD2Loader::Triangle triangle;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = 1;
    triangle.vertex[2] = 2;
    const std::vector<MD2Loader::Triangle> triangles{triangle};

    const auto targets = MD2Loader::buildMorphTargets(frames, triangles);
    REQUIRE(targets.positions.size() == 2);
    REQUIRE(targets.normals.size() == 2);
    REQUIRE(targets.names.size() == 2);
    REQUIRE(targets.names[0] == "a");
    REQUIRE(targets.names[1] == "b");
    // Three vertices times three components.
    REQUIRE(targets.positions[0].size() == 9);
    REQUIRE(targets.normals[0].size() == 9);
}

TEST_CASE("MD2Loader expands the triangle indices into the attribute order") {
    std::vector<MD2Loader::Frame> frames{makeFrame("a", 0)};
    // The triangle visits the vertices out of order, so the expansion must follow it.
    MD2Loader::Triangle triangle;
    triangle.vertex[0] = 2;
    triangle.vertex[1] = 0;
    triangle.vertex[2] = 1;
    const std::vector<MD2Loader::Triangle> triangles{triangle};

    const auto targets = MD2Loader::buildMorphTargets(frames, triangles);
    REQUIRE(targets.positions[0].size() == 9);
    // Vertex 2 is first in the triangle, so its x (2) leads.
    REQUIRE(targets.positions[0][0] == 2.0);
    REQUIRE(targets.positions[0][3] == 0.0);
    REQUIRE(targets.positions[0][6] == 1.0);
    // Frame two of the same shape is offset by a hundred.
    REQUIRE(targets.positions[0][1] == 12.0);
}

TEST_CASE("MD2Loader carries the normals into the morph target") {
    std::vector<MD2Loader::Frame> frames{makeFrame("a", 0)};
    MD2Loader::Triangle triangle;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = 1;
    triangle.vertex[2] = 2;
    const auto targets = MD2Loader::buildMorphTargets(frames, {triangle});
    // Every vertex of this frame has the same normal.
    for (int i = 0; i < 9; i += 3) {
        REQUIRE(targets.normals[0][static_cast<std::size_t>(i)] == 0.0);
        REQUIRE(targets.normals[0][static_cast<std::size_t>(i) + 2] == 1.0);
    }
}

TEST_CASE("MD2Loader skips a frame with no vertices") {
    std::vector<MD2Loader::Frame> frames;
    MD2Loader::Frame empty;
    empty.header.name = "empty";
    frames.push_back(empty);
    frames.push_back(makeFrame("real", 0));
    MD2Loader::Triangle triangle;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = 1;
    triangle.vertex[2] = 2;
    const auto targets = MD2Loader::buildMorphTargets(frames, {triangle});
    // The empty frame contributes no target at all.
    REQUIRE(targets.positions.size() == 1);
    REQUIRE(targets.names[0] == "real");
}

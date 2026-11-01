#include "loaders/MD2Loader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("MD2Loader exposes the full normal table") {
    const auto& table = MD2Loader::normalTable();
    REQUIRE(table.size() == 162);
    // The first entry is the upstream table's first row.
    REQUIRE(table[0][0] == Approx(-0.525731).epsilon(1e-9));
    REQUIRE(table[0][1] == Approx(0.0).margin(1e-12));
    REQUIRE(table[0][2] == Approx(0.850651).epsilon(1e-9));
    // Every row is a unit vector, which is what makes the table usable as normals.
    for (const auto& row : table) {
        const double length = row[0] * row[0] + row[1] * row[1] + row[2] * row[2];
        REQUIRE(length == Approx(1.0).margin(1e-4));
    }
}

TEST_CASE("MD2Loader scales and translates a vertex then swaps to Y-up") {
    // One vertex: quantised 128, 64, 32 with normal index zero.
    const std::vector<std::uint8_t> data{128, 64, 32, 0};
    const double scale[3] = {2.0, 2.0, 2.0};
    const double translation[3] = {0.0, 0.0, 0.0};
    std::vector<MD2Loader::Vertex> vertices;
    const std::size_t consumed = MD2Loader::decodeVertices(data, 0, 1, scale, translation, vertices);
    REQUIRE(consumed == 4);
    REQUIRE(vertices.size() == 1);
    // Scaled: 256, 128, 64. Then the last two axes swap, so y and z exchange.
    REQUIRE(vertices[0].position[0] == 256.0);
    REQUIRE(vertices[0].position[1] == 64.0);
    REQUIRE(vertices[0].position[2] == 128.0);
}

TEST_CASE("MD2Loader applies the frame translation before swapping") {
    const std::vector<std::uint8_t> data{1, 2, 3, 0};
    const double scale[3] = {10.0, 10.0, 10.0};
    const double translation[3] = {5.0, 6.0, 7.0};
    std::vector<MD2Loader::Vertex> vertices;
    MD2Loader::decodeVertices(data, 0, 1, scale, translation, vertices);
    // x = 1 * 10 + 5, y = 2 * 10 + 6 = 26, z = 3 * 10 + 7 = 37; after the swap y is 37.
    REQUIRE(vertices[0].position[0] == 15.0);
    REQUIRE(vertices[0].position[1] == 37.0);
    REQUIRE(vertices[0].position[2] == 26.0);
}

TEST_CASE("MD2Loader swaps the normal axes too") {
    const std::vector<std::uint8_t> data{0, 0, 0, 0};
    const double scale[3] = {1.0, 1.0, 1.0};
    const double translation[3] = {0.0, 0.0, 0.0};
    std::vector<MD2Loader::Vertex> vertices;
    MD2Loader::decodeVertices(data, 0, 1, scale, translation, vertices);
    // Table row zero is (-0.525731, 0, 0.850651); swapping the last two gives the same order
    // of magnitude with the middle and last exchanged.
    const auto& table = MD2Loader::normalTable()[0];
    REQUIRE(vertices[0].normal[0] == Approx(table[0]).epsilon(1e-9));
    REQUIRE(vertices[0].normal[1] == Approx(table[2]).epsilon(1e-9));
    REQUIRE(vertices[0].normal[2] == Approx(table[1]).epsilon(1e-9));
}

TEST_CASE("MD2Loader reads several vertices in sequence") {
    const std::vector<std::uint8_t> data{0, 0, 0, 0, 1, 1, 1, 1};
    const double scale[3] = {1.0, 1.0, 1.0};
    const double translation[3] = {0.0, 0.0, 0.0};
    std::vector<MD2Loader::Vertex> vertices;
    const std::size_t consumed = MD2Loader::decodeVertices(data, 0, 2, scale, translation, vertices);
    REQUIRE(consumed == 8);
    REQUIRE(vertices.size() == 2);
    REQUIRE(vertices[1].position[0] == 1.0);
}

TEST_CASE("MD2Loader fails when the vertex data ends early") {
    const std::vector<std::uint8_t> data{0, 0};
    const double scale[3] = {1.0, 1.0, 1.0};
    const double translation[3] = {0.0, 0.0, 0.0};
    std::vector<MD2Loader::Vertex> vertices;
    REQUIRE_THROWS_WITH(MD2Loader::decodeVertices(data, 0, 1, scale, translation, vertices),
                        "THREE.MD2Loader: Unexpected end of data");
}

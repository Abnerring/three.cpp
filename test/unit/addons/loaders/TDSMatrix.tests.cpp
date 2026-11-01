#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <array>

TEST_CASE("TDSLoader arranges the mesh matrix") {
    // Twelve distinct values so every element can be told apart.
    std::array<double, 12> values{};
    for (int i = 0; i < 12; ++i) values[static_cast<std::size_t>(i)] = i + 1;
    const auto matrix = TDSLoader::meshMatrix(values);

    // The first column takes the first, third and second values, then the padding of zero.
    REQUIRE(matrix[0] == 1);
    REQUIRE(matrix[1] == 3);
    REQUIRE(matrix[2] == 2);
    REQUIRE(matrix[3] == 0);
    // The second column takes the seventh, ninth and eighth.
    REQUIRE(matrix[4] == 7);
    REQUIRE(matrix[5] == 9);
    REQUIRE(matrix[6] == 8);
    REQUIRE(matrix[7] == 0);
    // The third column takes the fourth, sixth and fifth.
    REQUIRE(matrix[8] == 4);
    REQUIRE(matrix[9] == 6);
    REQUIRE(matrix[10] == 5);
    REQUIRE(matrix[11] == 0);
    // The last column holds the translation and the constant one.
    REQUIRE(matrix[12] == 10);
    REQUIRE(matrix[13] == 12);
    REQUIRE(matrix[14] == 11);
    REQUIRE(matrix[15] == 1);
}

TEST_CASE("TDSLoader leaves an identity matrix alone") {
    // A three by four that describes no transform must give the identity.
    const std::array<double, 12> identity{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    const auto matrix = TDSLoader::meshMatrix(identity);
    REQUIRE(matrix[0] == 1);
    REQUIRE(matrix[1] == 0);
    REQUIRE(matrix[2] == 0);
    REQUIRE(matrix[5] == 1);
    REQUIRE(matrix[10] == 1);
    REQUIRE(matrix[15] == 1);
}

TEST_CASE("TDSLoader keeps a translation in the last column") {
    // Only the translation values are non zero here.
    const std::array<double, 12> moved{1, 0, 0, 0, 1, 0, 0, 0, 1, 5, 6, 7};
    const auto matrix = TDSLoader::meshMatrix(moved);
    // Five, six and seven land in the fourth column of the first three rows.
    REQUIRE(matrix[12] == 5);
    REQUIRE(matrix[13] == 7);
    REQUIRE(matrix[14] == 6);
    // The three by three part is still the identity.
    REQUIRE(matrix[0] == 1);
    REQUIRE(matrix[5] == 1);
    REQUIRE(matrix[10] == 1);
}

#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("VOXLoader merges a solid block into one quad") {
    // Two by two, all the same value, so one rectangle covers it.
    const std::vector<int> mask{7, 7, 7, 7};
    const auto quads = VOXLoader::mergeMask(mask, 2, 2);
    REQUIRE(quads.size() == 1);
    REQUIRE(quads[0].u == 0);
    REQUIRE(quads[0].v == 0);
    REQUIRE(quads[0].w == 2);
    REQUIRE(quads[0].h == 2);
    REQUIRE(quads[0].value == 7);
}

TEST_CASE("VOXLoader keeps a whole row before growing downwards") {
    // The first row matches but the second does not, so two one by one quads below a one by two.
    const std::vector<int> mask{7, 7,
                                0, 0};
    const auto quads = VOXLoader::mergeMask(mask, 2, 2);
    REQUIRE(quads.size() == 1);
    REQUIRE(quads[0].w == 2);
    REQUIRE(quads[0].h == 1);
}

TEST_CASE("VOXLoader does not merge different values") {
    // Four distinct values, so each cell stands alone.
    const std::vector<int> mask{1, 2,
                                3, 4};
    const auto quads = VOXLoader::mergeMask(mask, 2, 2);
    REQUIRE(quads.size() == 4);
    REQUIRE(quads[0].value == 1);
    REQUIRE(quads[3].value == 4);
    // Each is a single cell.
    for (const auto& quad : quads) {
        REQUIRE(quad.w == 1);
        REQUIRE(quad.h == 1);
    }
}

TEST_CASE("VOXLoader merges columns of the same sign") {
    // A vertical run of negative faces merges into a one wide, three tall quad.
    const std::vector<int> mask{-5, -5, -5};
    const auto quads = VOXLoader::mergeMask(mask, 1, 3);
    REQUIRE(quads.size() == 1);
    REQUIRE(quads[0].w == 1);
    REQUIRE(quads[0].h == 3);
    // The sign survives, which is what decides the facing later.
    REQUIRE(quads[0].value == -5);
}

TEST_CASE("VOXLoader skips empty cells") {
    const std::vector<int> mask{0, 0, 0, 0};
    REQUIRE(VOXLoader::mergeMask(mask, 2, 2).empty());
}

TEST_CASE("VOXLoader merges an L shape into two rectangles") {
    // Three cells in an L: the top row of two, then one below the first column.
    const std::vector<int> mask{3, 3,
                                3, 0};
    const auto quads = VOXLoader::mergeMask(mask, 2, 2);
    REQUIRE(quads.size() == 2);
    // The first covers both columns of the top row.
    REQUIRE(quads[0].w == 2);
    REQUIRE(quads[0].h == 1);
    // The second covers the single cell below.
    REQUIRE(quads[1].u == 0);
    REQUIRE(quads[1].v == 1);
    REQUIRE(quads[1].w == 1);
}


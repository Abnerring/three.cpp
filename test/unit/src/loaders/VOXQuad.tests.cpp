#include "loaders/VOXLoader.h"

#include "catch.hpp"

namespace {
VOXLoader::Size makeSize(int x, int y, int z) {
    VOXLoader::Size size;
    size.x = x;
    size.y = y;
    size.z = z;
    return size;
}
}  // namespace

TEST_CASE("VOXLoader centres and reorders a point") {
    const auto size = makeSize(4, 2, 6);
    const int point[3] = {0, 0, 0};
    const auto mapped = VOXLoader::toThree(point, size);
    // x keeps, y takes the old z and z takes the negated old y, then half of each extent is
    // subtracted or added.
    REQUIRE(mapped[0] == -2.0);
    REQUIRE(mapped[1] == -3.0);
    REQUIRE(mapped[2] == 1.0);
}

TEST_CASE("VOXLoader emits four corners and six indices") {
    const auto size = makeSize(4, 4, 4);
    VOXLoader::Quad quad;
    quad.u = 0;
    quad.v = 0;
    quad.w = 2;
    quad.h = 1;
    quad.value = 5;
    // Along z at slice one, spanning two in x.
    const int pos[3] = {0, 0, 1};
    const int du[3] = {2, 0, 0};
    const int dv[3] = {0, 1, 0};
    const auto geometry = VOXLoader::emitQuad(quad, pos, du, dv, size);
    // Four corners times three components.
    REQUIRE(geometry.vertices[0] == -2.0);
    REQUIRE(geometry.vertices[1] == -1.0);
    // The index list is two triangles sharing an edge.
    REQUIRE(geometry.indices[0] == 0);
    REQUIRE(geometry.indices[2] == 2);
    REQUIRE(geometry.indices[3] == 0);
    REQUIRE(geometry.indices[5] == 3);
}

TEST_CASE("VOXLoader reverses the corners for a negative face") {
    const auto size = makeSize(2, 2, 2);
    VOXLoader::Quad positive;
    positive.value = 5;
    positive.w = 1;
    positive.h = 1;
    VOXLoader::Quad negative = positive;
    negative.value = -5;

    const int pos[3] = {0, 0, 0};
    const int du[3] = {1, 0, 0};
    const int dv[3] = {0, 1, 0};
    const auto front = VOXLoader::emitQuad(positive, pos, du, dv, size);
    const auto back = VOXLoader::emitQuad(negative, pos, du, dv, size);

    // The first corner is the same, the middle two swap.
    REQUIRE(front.vertices[0] == back.vertices[0]);
    REQUIRE(front.vertices[3] == back.vertices[9]);
    REQUIRE(front.vertices[6] == back.vertices[6]);
    REQUIRE(front.vertices[9] == back.vertices[3]);
    // The indices themselves are identical in both cases.
    for (std::size_t i = 0; i < 6; ++i) REQUIRE(front.indices[i] == back.indices[i]);
}

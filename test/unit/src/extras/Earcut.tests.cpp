#include "catch.hpp"
#include "extras/lib/earcut.h"
#include "core/JSValue.h"
#include <numbers>

TEST_CASE("Earcut preserves exact upstream triangle order for winding and concavity", "[extras][earcut]") {
    const std::vector<double> square{0,0,10,0,10,10,0,10};
    REQUIRE((earcut(square) == std::vector<std::size_t>{2,3,0,0,1,2}));
    REQUIRE((earcut({0,10,10,10,10,0,0,0}) == std::vector<std::size_t>{1,0,3,3,2,1}));
    REQUIRE((earcut({0,0,4,0,4,4,2,1,0,4}) == std::vector<std::size_t>{3,4,0,1,2,3,3,0,1}));
    REQUIRE((earcut({0,0,1,10,0,2,10,10,3,0,10,4},{},3) == std::vector<std::size_t>{2,3,0,0,1,2}));
    REQUIRE(deviation(square,{},2,earcut(square)) == 0);
}

TEST_CASE("Earcut holes and Steiner points match pinned bridge and face order", "[extras][earcut]") {
    const std::vector<double> ring{0,0,10,0,10,10,0,10,2,2,2,8,8,8,8,2};
    const auto triangles = earcut(ring,{4});
    REQUIRE((triangles == std::vector<std::size_t>{0,4,5,7,4,0,3,0,5,7,0,1,2,3,5,6,7,1,2,5,6,6,1,2}));
    REQUIRE(deviation(ring,{4},2,triangles) == 0);
    REQUIRE((earcut({0,0,10,0,10,10,0,10,2,2},{4}) == std::vector<std::size_t>{4,0,1,1,2,3,3,0,4,4,1,3}));
    REQUIRE_THROWS_AS(earcut({0,0,10,0,10,10,0,10},{4}), JSTypeError);
}

TEST_CASE("Earcut duplicate collinear degenerate and self intersecting contours retain upstream behavior", "[extras][earcut]") {
    REQUIRE(earcut({}).empty());
    REQUIRE(earcut({0,0}).empty());
    REQUIRE(earcut({0,0,1,1}).empty());
    REQUIRE(earcut({0,0,1,1,2,2,3,3}).empty());
    REQUIRE(earcut({0,0,0,0,0,0}).empty());
    REQUIRE((earcut({0,0,10,10,0,10,10,0}) == std::vector<std::size_t>{1,0,3}));
    REQUIRE(deviation({}, {}, 2, {}) == 0);
    REQUIRE_THROWS_AS(earcut({0,0,1,0,0,1},{},0), JSRangeError);
}

TEST_CASE("Earcut z order triangulation preserves indices and area on large polygons", "[extras][earcut]") {
    std::vector<double> ring;
    constexpr std::size_t count = 120;
    for (std::size_t i = 0; i < count; ++i) {
        const auto angle = static_cast<double>(i) * 2 * std::numbers::pi / count;
        ring.push_back(10 * std::cos(angle)); ring.push_back(10 * std::sin(angle));
    }
    const auto triangles = earcut(ring);
    REQUIRE(triangles.size() == (count - 2) * 3);
    for (const auto index : triangles) REQUIRE(index < count);
    REQUIRE(deviation(ring,{},2,triangles) < 1e-12);
    REQUIRE(earcut(ring) == triangles);
}

TEST_CASE("Earcut flatten preserves ring order dimensions and upstream empty-ring offsets", "[extras][earcut]") {
    const std::vector<std::vector<std::vector<double>>> rings{
        {{0,0,1},{1,0,2},{0,1,3}}, {{.2,.2,4}}};
    const auto result = flatten(rings);
    REQUIRE(result.dimensions == 3);
    REQUIRE((result.vertices == std::vector<double>{0,0,1,1,0,2,0,1,3,.2,.2,4}));
    REQUIRE((result.holes == std::vector<std::size_t>{3}));
    const auto emptyMiddle = flatten({{{0,0},{1,0},{0,1}}, {}, {{.2,.2}}});
    REQUIRE((emptyMiddle.holes == std::vector<std::size_t>{3}));
    REQUIRE_THROWS_AS(flatten({}), JSTypeError);
    REQUIRE_THROWS_AS(flatten({{}}), JSTypeError);
}

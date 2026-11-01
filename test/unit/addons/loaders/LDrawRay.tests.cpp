#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <array>

TEST_CASE("LDrawLoader normalises an edge into a ray") {
    // A segment along the x axis, from one to three.
    const std::array<double, 3> v0{1, 0, 0};
    const std::array<double, 3> v1{3, 0, 0};
    const auto ray = LDrawLoader::toNormalizedRay(v0, v1);
    // The direction is unit length along the axis.
    REQUIRE(ray.direction[0] == Approx(1.0).epsilon(1e-12));
    REQUIRE(ray.direction[1] == 0.0);
    // The origin is the foot of the perpendicular from the world origin, which is zero here.
    REQUIRE(ray.origin[0] == Approx(0.0).margin(1e-12));
    REQUIRE(ray.origin[1] == 0.0);
    REQUIRE(ray.origin[2] == 0.0);
}

TEST_CASE("LDrawLoader gives both directions the same origin") {
    // Read from the other end the direction flips but the line is the same.
    const std::array<double, 3> a{1, 0, 0};
    const std::array<double, 3> b{3, 0, 0};
    const auto forward = LDrawLoader::toNormalizedRay(a, b);
    const auto backward = LDrawLoader::toNormalizedRay(b, a);
    // The directions are opposite.
    REQUIRE(forward.direction[0] == Approx(-backward.direction[0]).epsilon(1e-12));
    // The origins agree, which is the point of normalising.
    for (int i = 0; i < 3; ++i) {
        REQUIRE(forward.origin[static_cast<std::size_t>(i)] ==
                Approx(backward.origin[static_cast<std::size_t>(i)]).margin(1e-12));
    }
}

TEST_CASE("LDrawLoader normalises a diagonal edge") {
    // A segment along the diagonal from one to two on every axis.
    const std::array<double, 3> v0{1, 1, 1};
    const std::array<double, 3> v1{2, 2, 2};
    const auto ray = LDrawLoader::toNormalizedRay(v0, v1);
    // Each component is one over the square root of three.
    const double expected = 1.0 / std::sqrt(3.0);
    REQUIRE(ray.direction[0] == Approx(expected).epsilon(1e-12));
    REQUIRE(ray.direction[1] == Approx(expected).epsilon(1e-12));
    REQUIRE(ray.direction[2] == Approx(expected).epsilon(1e-12));
}

TEST_CASE("LDrawLoader hashes a vertex by truncation") {
    // A multiplier of ten turns the coordinates into small integers.
    const std::array<double, 3> v{1.25, -2.75, 0.5};
    REQUIRE(LDrawLoader::hashVertex(v, 10.0) == "12,-27,5");
    // Truncation goes towards zero in both directions.
    const std::array<double, 3> negative{-1.9, -0.1, 0.0};
    REQUIRE(LDrawLoader::hashVertex(negative, 1.0) == "-1,0,0");
}

TEST_CASE("LDrawLoader hashes an edge in the order given") {
    const std::array<double, 3> a{1, 0, 0};
    const std::array<double, 3> b{3, 0, 0};
    // The two orders are two different keys, which is why both are recorded.
    REQUIRE(LDrawLoader::hashEdge(a, b, 10.0) == "10,0,0_30,0,0");
    REQUIRE(LDrawLoader::hashEdge(b, a, 10.0) == "30,0,0_10,0,0");
    REQUIRE(LDrawLoader::hashEdge(a, b, 10.0) != LDrawLoader::hashEdge(b, a, 10.0));
}

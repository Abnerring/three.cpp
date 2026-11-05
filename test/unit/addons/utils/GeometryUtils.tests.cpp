#include "utils/GeometryUtils.h"

#include "math/Vector3.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("GeometryUtils hilbert2D follows the recursive corner order") {
    const auto points = GeometryUtils::hilbert2D(Vector3(0, 0, 0), 10, 1);
    // One iteration expands to four sub-curves of four points each.
    REQUIRE(points.size() == 16);
    // One recursion halves the size, so the first point is the first corner of the first sub-curve.
    REQUIRE(points.front().x == Approx(-7.5));
    REQUIRE(points.front().y == Approx(0));
    REQUIRE(points.front().z == Approx(-7.5));

    const auto deeper = GeometryUtils::hilbert2D(Vector3(0, 0, 0), 10, 2);
    REQUIRE(deeper.size() == 64);
}

TEST_CASE("GeometryUtils hilbert3D follows the recursive corner order") {
    const auto points = GeometryUtils::hilbert3D(Vector3(0, 0, 0), 10, 1);
    // One iteration expands to eight sub-curves of eight points each.
    REQUIRE(points.size() == 64);
    // One recursion halves the size, so the first point is the first corner of the first sub-curve.
    REQUIRE(points.front().x == Approx(-7.5));
    REQUIRE(points.front().y == Approx(7.5));
    REQUIRE(points.front().z == Approx(-7.5));
}

TEST_CASE("GeometryUtils gosper emits the expanding flowsnake path") {
    const auto path = GeometryUtils::gosper(1);
    // The path starts at the origin and every F adds one xyz triple.
    REQUIRE(path.size() >= 3);
    REQUIRE(path[0] == Approx(0));
    REQUIRE(path[1] == Approx(0));
    REQUIRE(path[2] == Approx(0));

    // The generator is deterministic and scales linearly with the size argument.
    const auto doubled = GeometryUtils::gosper(2);
    REQUIRE(doubled.size() == path.size());
    for (std::size_t i = 0; i + 2 < path.size(); i += 3) {
        REQUIRE(doubled[i] == Approx(path[i] * 2).margin(1e-12));
        REQUIRE(doubled[i + 1] == Approx(path[i + 1] * 2).margin(1e-12));
        REQUIRE(doubled[i + 2] == Approx(0));
    }
}

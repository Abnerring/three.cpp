#include "catch.hpp"
#include "core/JSValue.h"
#include "extras/Earcut.h"
#include <vector>

// Upstream is a static wrapper over the bundled earcut, so the wrapper has to give the same indices for the
// same input: a square is two triangles, and a square with a hole is more.
TEST_CASE("Earcut triangulates through the bundled algorithm") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const std::vector<double> square{0, 0, 1, 0, 1, 1, 0, 1};
    const auto faces = Earcut::triangulate(square);
    REQUIRE(faces.size() == 6);
    for (const auto index : faces) REQUIRE(index < 4);

    // A hole pushes the outer ring and the hole apart, so the index count grows with both.
    const std::vector<double> ring{0, 0, 10, 0, 10, 10, 0, 10, 2, 2, 4, 2, 4, 4, 2, 4};
    const std::vector<std::size_t> holes{4};
    const auto holed = Earcut::triangulate(ring, holes);
    REQUIRE(holed.size() > faces.size());
    for (const auto index : holed) REQUIRE(index < 8);

    // The JS form takes arrays and defaults the dimension to two.
    auto points = JSValue::array();
    for (std::size_t i = 0; i < square.size(); i += 1) points.set(static_cast<double>(i), JSValue(square[i]));
    const auto jsFaces = Earcut::triangulate(points);
    REQUIRE(jsFaces.get("length").toNumber() == 6.0);
}
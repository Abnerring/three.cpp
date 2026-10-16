#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/BoxLineGeometry.h"
#include <cmath>
#include <memory>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/BoxLineGeometry.js under the pinned Node oracle.
namespace {
float component(const BufferGeometry& geometry, std::size_t index) {
    return std::get<Float32Array>(
        std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute("position"))->array)[index];
}
}

TEST_CASE("BoxLineGeometry emits four segments per grid line", "[boxlinegeometry]") {
    BoxLineGeometry geometry(2, 3, 4, 2, 1, 3);
    REQUIRE(geometry.type == "BufferGeometry"); // the pinned class never sets a type
    const auto position = geometry.getAttribute("position");
    // Four vertex pairs per segment step, over ( ws + 1 ) + ( hs + 1 ) + ( ds + 1 ) steps.
    REQUIRE(position->getCount() == 4 * 2 * ((2 + 1) + (1 + 1) + (3 + 1)));
    REQUIRE(geometry.index == nullptr);
    // The oracle's first vertices: x = -widthHalf, from y = -heightHalf to +heightHalf.
    REQUIRE(component(geometry, 0) == -1.f);
    REQUIRE(component(geometry, 1) == -1.5f);
    REQUIRE(component(geometry, 2) == -2.f);
    REQUIRE(component(geometry, 3) == -1.f);
    REQUIRE(component(geometry, 4) == 1.5f);
    REQUIRE(component(geometry, 5) == -2.f);
    REQUIRE(component(geometry, 6) == -1.f);
    REQUIRE(component(geometry, 7) == 1.5f);
    REQUIRE(component(geometry, 8) == -2.f);
}

TEST_CASE("BoxLineGeometry floors fractional segment counts", "[boxlinegeometry]") {
    BoxLineGeometry floored(1, 1, 1, 2.9, 1.2, 0.7);
    BoxLineGeometry integers(1, 1, 1, 2, 1, 0);
    REQUIRE(floored.getAttribute("position")->getCount() == integers.getAttribute("position")->getCount());
    REQUIRE(floored.getAttribute("position")->getCount() == 4 * 2 * (3 + 2 + 1));
    // A zero segment count divides by zero like the pinned source; the infinite step only
    // matters for a following iteration, so the single emitted step keeps finite corners.
    BoxLineGeometry zero(1, 1, 1, 0, 0, 0);
    REQUIRE(zero.getAttribute("position")->getCount() == 4 * 2 * 3);
    REQUIRE(component(zero, 0) == -0.5f);
    REQUIRE(component(zero, 2) == -0.5f);
    REQUIRE(std::isfinite(component(zero, 3)));
}

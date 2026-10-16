#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/RoundedBoxGeometry.h"
#include <cmath>
#include <memory>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/RoundedBoxGeometry.js under the pinned Node oracle.
namespace {
const double epsilon = 1e-6;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
float component(const BufferGeometry& geometry, const std::string& name, std::size_t index) {
    return std::get<Float32Array>(
        std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name))->array)[index];
}
}

TEST_CASE("RoundedBoxGeometry rounds the unit box into a non-indexed geometry", "[roundedboxgeometry]") {
    RoundedBoxGeometry geometry(1, 1, 1, 2, 0.1);
    REQUIRE(geometry.type == "RoundedBoxGeometry");
    REQUIRE(geometry.parameters.width == 1.);
    REQUIRE(geometry.parameters.height == 1.);
    REQUIRE(geometry.parameters.depth == 1.);
    REQUIRE(geometry.parameters.segments == 2.);
    REQUIRE(close(geometry.parameters.radius, 0.1));
    // The pinned class replaces the index with the non-indexed attribute arrays.
    REQUIRE(geometry.index == nullptr);
    REQUIRE(geometry.getAttribute("position")->getCount() == 900);
    REQUIRE(geometry.getAttribute("normal")->getCount() == 900);
    REQUIRE(geometry.getAttribute("uv")->getCount() == 900);
    // The oracle's first rounded corner and its normal.
    REQUIRE(close(component(geometry, "position", 0), 0.4577350318431854));
    REQUIRE(close(component(geometry, "position", 1), 0.4577350318431854));
    REQUIRE(close(component(geometry, "position", 2), 0.4577350318431854));
    REQUIRE(close(component(geometry, "uv", 1), 1.));
    REQUIRE(close(component(geometry, "uv", 2), 0.));
    REQUIRE(close(component(geometry, "uv", 3), 0.9663820266723633));
    // Every normal stays unit length.
    for (std::size_t i = 0; i < 900; ++i) {
        const double x = component(geometry, "normal", i * 3 + 0);
        const double y = component(geometry, "normal", i * 3 + 1);
        const double z = component(geometry, "normal", i * 3 + 2);
        REQUIRE(close(std::sqrt(x * x + y * y + z * z), 1.));
    }
}

TEST_CASE("RoundedBoxGeometry keeps the plain box when totalSegments is one", "[roundedboxgeometry]") {
    // segments = 0 gives totalSegments = 1, so the pinned constructor returns early and the
    // indexed unit box stays in place.
    RoundedBoxGeometry geometry(1, 1, 1, 0, 0.1);
    REQUIRE(geometry.type == "RoundedBoxGeometry");
    REQUIRE(geometry.index != nullptr);
    REQUIRE(geometry.getAttribute("position")->getCount() == 24);
    REQUIRE(geometry.index->getCount() == 36);
}

TEST_CASE("RoundedBoxGeometry clamps the radius to the smallest side", "[roundedboxgeometry]") {
    RoundedBoxGeometry geometry(1, 1, 1, 2, 2);
    REQUIRE(close(geometry.parameters.radius, 0.5));
    REQUIRE(geometry.getAttribute("position")->getCount() == 900);
    RoundedBoxGeometry wide(2, 1, 3, 1, 0.25);
    REQUIRE(close(wide.parameters.radius, 0.25));
    REQUIRE(wide.parameters.segments == 1.);
}

TEST_CASE("RoundedBoxGeometry JSON and fromJSON keep the pinned fields", "[roundedboxgeometry]") {
    RoundedBoxGeometry geometry(2, 1, 3, 1, 0.25);
    const JsonValue json = geometry.toJSON();
    REQUIRE(json.at("type").get<std::string>() == "RoundedBoxGeometry");
    REQUIRE(json.at("width").get<double>() == 2.);
    REQUIRE(json.at("height").get<double>() == 1.);
    REQUIRE(json.at("depth").get<double>() == 3.);
    REQUIRE(json.at("segments").get<double>() == 1.);
    REQUIRE(close(json.at("radius").get<double>(), 0.25));
    auto restored = RoundedBoxGeometry::fromJSON(json);
    REQUIRE(restored->parameters.width == 2.);
    REQUIRE(restored->parameters.depth == 3.);
    REQUIRE(close(restored->parameters.radius, 0.25));
    REQUIRE(restored->getAttribute("position")->getCount() == geometry.getAttribute("position")->getCount());
    // Like the pinned class, copy() is not overridden, so a clone keeps default parameters
    // while copying the vertex data.
    auto clone = std::dynamic_pointer_cast<RoundedBoxGeometry>(geometry.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->parameters.width == 1.);
    REQUIRE(clone->parameters.segments == 2.);
    REQUIRE(clone->getAttribute("position")->getCount() == geometry.getAttribute("position")->getCount());
}

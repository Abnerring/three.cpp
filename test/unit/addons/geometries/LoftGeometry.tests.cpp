#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/LoftGeometry.h"
#include <cmath>
#include <memory>

// The reference values in this file come from the pinned examples/jsm/geometries/LoftGeometry.js
// (they are the same values the differential probe compares bit for bit).
namespace {
LoftGeometry::Section squareRing(double z) {
    return {Vector3(-1, -1, z), Vector3(1, -1, z), Vector3(1, 1, z), Vector3(-1, 1, z)};
}
double component(const BufferGeometry& geometry, const std::string& name, std::size_t index) {
    return std::get<Float32Array>(
        std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name))->array)[index];
}
// `component` indexes the raw array; this helper indexes a vertex of an attribute.
double vertexComponent(const BufferGeometry& geometry, const std::string& name, std::size_t vertex,
                       std::size_t component) {
    const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name));
    return std::get<Float32Array>(attribute->array)[vertex * std::size_t(attribute->itemSize) + component];
}
}

TEST_CASE("LoftGeometry builds the pinned closed tube", "[loftgeometry]") {
    LoftGeometry geometry({squareRing(0), squareRing(2)});
    REQUIRE(geometry.type == "LoftGeometry");
    REQUIRE(geometry.parameters.closed);
    REQUIRE_FALSE(geometry.parameters.capStart);
    REQUIRE_FALSE(geometry.parameters.capEnd);
    // Closed sections repeat their first point: rows * ( columns + 1 ) vertices.
    REQUIRE(geometry.getAttribute("position")->getCount() == 10);
    REQUIRE(geometry.getAttribute("uv")->getCount() == 10);
    REQUIRE(geometry.getAttribute("normal")->getCount() == 10);
    REQUIRE(geometry.index != nullptr);
    REQUIRE(geometry.index->getCount() == 24); // ( rows - 1 ) * columns * 6
    REQUIRE(component(geometry, "position", 0) == -1.f);
    REQUIRE(component(geometry, "position", 2) == 0.f);
    REQUIRE(component(geometry, "position", 3) == 1.f);
    // The repeated seam vertex duplicates the first point of its row: row 0 covers vertices
    // 0..4 and row 1 covers 5..9, so the last vertex of each row repeats its first one.
    for (std::size_t c = 0; c < 3; ++c) {
        REQUIRE(vertexComponent(geometry, "position", 4, c) == vertexComponent(geometry, "position", 0, c));
        REQUIRE(vertexComponent(geometry, "position", 9, c) == vertexComponent(geometry, "position", 5, c));
    }
    // The seam normals are averaged, so both copies of a seam share one normal.
    for (std::size_t c = 0; c < 3; ++c) {
        REQUIRE(vertexComponent(geometry, "normal", 4, c) == vertexComponent(geometry, "normal", 0, c));
        REQUIRE(vertexComponent(geometry, "normal", 9, c) == vertexComponent(geometry, "normal", 5, c));
    }
    const double length = std::sqrt(component(geometry, "normal", 0) * component(geometry, "normal", 0) +
                                    component(geometry, "normal", 1) * component(geometry, "normal", 1) +
                                    component(geometry, "normal", 2) * component(geometry, "normal", 2));
    REQUIRE(std::abs(length - 1.) < 1e-6);
}

TEST_CASE("LoftGeometry open sections and caps follow the pinned layout", "[loftgeometry]") {
    LoftGeometry open({squareRing(0), squareRing(2)}, LoftGeometry::Options{false, false, false});
    REQUIRE(open.getAttribute("position")->getCount() == 8);
    REQUIRE(open.index->getCount() == 18); // ( rows - 1 ) * ( columns - 1 ) * 6
    // Caps append their own vertices: the two ring caps add 4 vertices each.
    LoftGeometry caps({squareRing(0), squareRing(2)}, LoftGeometry::Options{true, true, true});
    REQUIRE(caps.getAttribute("position")->getCount() == 10 + 4 + 4);
    REQUIRE(caps.getAttribute("uv")->getCount() == caps.getAttribute("position")->getCount());
    REQUIRE(caps.index->getCount() > 24);
    for (std::size_t i = 0; i < caps.getAttribute("uv")->getCount(); ++i)
        REQUIRE(component(caps, "uv", i) >= 0.f);
}

TEST_CASE("LoftGeometry reports the pinned guards and copies parameters", "[loftgeometry]") {
    // Fewer than two sections: the geometry keeps no attributes.
    LoftGeometry single({squareRing(0)});
    REQUIRE(single.type == "LoftGeometry");
    REQUIRE(single.getAttribute("position") == nullptr);
    REQUIRE(single.index == nullptr);
    LoftGeometry empty;
    REQUIRE(empty.getAttribute("position") == nullptr);
    // Ragged sections are rejected the same way.
    LoftGeometry ragged({squareRing(0), {Vector3(0, 0, 2), Vector3(1, 0, 2), Vector3(0, 1, 2)}});
    REQUIRE(ragged.getAttribute("position") == nullptr);
    // clone() copies the parameters through copy(), like the pinned Object.assign.
    LoftGeometry source({squareRing(0), squareRing(2)}, LoftGeometry::Options{false, true, false});
    auto clone = std::dynamic_pointer_cast<LoftGeometry>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->parameters.sections.size() == 2);
    REQUIRE(clone->parameters.closed == false);
    REQUIRE(clone->parameters.capStart);
    REQUIRE(clone->getAttribute("position")->getCount() == source.getAttribute("position")->getCount());
    const JsonValue json = source.toJSON();
    REQUIRE(json.at("type").get<std::string>() == "LoftGeometry");
    REQUIRE(json.at("sections").get<JsonValue::Array>().size() == 2);
    REQUIRE(json.at("closed").get<bool>() == false);
    REQUIRE(json.at("capStart").get<bool>());
    REQUIRE_FALSE(json.at("capEnd").get<bool>());
}

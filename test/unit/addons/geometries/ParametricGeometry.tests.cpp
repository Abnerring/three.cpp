#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/ParametricFunctions.h"
#include "geometries/ParametricGeometry.h"
#include "math/JSNumberMath.h"
#include "math/Vector3.h"
#include <cmath>
#include <memory>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/ParametricGeometry.js under the pinned Node oracle.
namespace {
const double epsilon = 1e-6;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
float component(const BufferGeometry& geometry, const std::string& name, std::size_t index) {
    return std::get<Float32Array>(
        std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name))->array)[index];
}
}

TEST_CASE("ParametricGeometry builds the pinned vertex and index layout", "[parametricgeometry]") {
    ParametricGeometry geometry(ParametricFunctions::klein, 5, 4);
    REQUIRE(geometry.type == "ParametricGeometry");
    REQUIRE(geometry.parameters.slices == 5);
    REQUIRE(geometry.parameters.stacks == 4);
    const auto position = geometry.getAttribute("position");
    REQUIRE(position->getCount() == 30); // ( stacks + 1 ) * ( slices + 1 )
    REQUIRE(position->getItemSize() == 3);
    REQUIRE(geometry.getAttribute("normal")->getCount() == 30);
    REQUIRE(geometry.getAttribute("uv")->getItemSize() == 2);
    REQUIRE(geometry.index != nullptr);
    REQUIRE(geometry.index->getCount() == 120); // stacks * slices * 6
    // The oracle's first vertices and their uv values.
    REQUIRE(close(component(geometry, "position", 0), 4.));
    REQUIRE(close(component(geometry, "position", 1), 0.));
    REQUIRE(close(component(geometry, "position", 3), 3.3090169429779053));
    REQUIRE(close(component(geometry, "position", 4), -0.9510565400123596));
    REQUIRE(close(component(geometry, "uv", 2), 0.20000000298023224));
    REQUIRE(close(component(geometry, "uv", 3), 0.));
}

TEST_CASE("ParametricGeometry uses the pinned default callback", "[parametricgeometry]") {
    // The default function is ( u, v, target ) => target.set( u, v, cos( u ) * sin( v ) ).
    ParametricGeometry geometry({}, 2, 2);
    REQUIRE(close(component(geometry, "position", 0), 0.));
    REQUIRE(close(component(geometry, "position", 1), 0.));
    REQUIRE(close(component(geometry, "position", 2), 0.));
    // Second vertex: u = 0.5, v = 0.
    REQUIRE(close(component(geometry, "position", 3), 0.5));
    REQUIRE(close(component(geometry, "position", 5), JSNumberMath::cos(0.5) * JSNumberMath::sin(0.)));
    // Third vertex: u = 1, v = 0.
    REQUIRE(close(component(geometry, "position", 6), 1.));
}

TEST_CASE("ParametricGeometry plane normals and JSON follow the pinned source", "[parametricgeometry]") {
    ParametricGeometry geometry(ParametricFunctions::plane, 1, 1);
    REQUIRE(component(geometry, "position", 0) == 0.f);
    REQUIRE(component(geometry, "position", 3) == 1.f);
    REQUIRE(component(geometry, "position", 6) == 0.f);
    REQUIRE(component(geometry, "position", 9) == 1.f);
    REQUIRE(geometry.index->getCount() == 6);
    REQUIRE(std::get<Uint16Array>(geometry.index->array)[0] == 0);
    REQUIRE(std::get<Uint16Array>(geometry.index->array)[5] == 2);
    const JsonValue json = geometry.toJSON();
    REQUIRE(json.at("type").get<std::string>() == "ParametricGeometry");
    REQUIRE(json.at("slices").get<double>() == 1.);
    REQUIRE(json.at("stacks").get<double>() == 1.);
    // `parameters.func` is a function value and never reaches the JSON object.
    REQUIRE_FALSE(json.contains("func"));
}

TEST_CASE("ParametricGeometry copy and clone keep the callback and the numeric fields", "[parametricgeometry]") {
    ParametricGeometry geometry(ParametricFunctions::mobius, 3, 2);
    auto clone = std::dynamic_pointer_cast<ParametricGeometry>(geometry.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->type == "ParametricGeometry");
    REQUIRE(clone->parameters.slices == 3);
    REQUIRE(clone->parameters.stacks == 2);
    REQUIRE(static_cast<bool>(clone->parameters.func));
    ParametricGeometry other(ParametricFunctions::plane, 4, 4);
    other.copy(geometry);
    REQUIRE(other.parameters.slices == 3);
    REQUIRE(other.parameters.stacks == 2);
    REQUIRE(static_cast<bool>(other.parameters.func));
    REQUIRE(other.getAttribute("position")->getCount() == geometry.getAttribute("position")->getCount());
}

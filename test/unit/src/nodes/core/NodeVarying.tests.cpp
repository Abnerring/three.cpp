#include "nodes/core/NodeVarying.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeVarying carries a name and a type") {
    const NodeVarying varying("vUv", "vec2");
    REQUIRE(varying.name() == "vUv");
    REQUIRE(varying.type() == "vec2");
    REQUIRE(NodeVarying::isNodeVarying);
}

TEST_CASE("NodeVarying's interpolation flag starts clear") {
    // The source sets the flag clear in its constructor rather than working it out.
    const NodeVarying varying("v", "float");
    REQUIRE_FALSE(varying.needsInterpolation());
    // It is set by whoever knows, so it can be turned on and off.
    NodeVarying other("v", "float");
    other.setNeedsInterpolation(true);
    REQUIRE(other.needsInterpolation());
    other.setNeedsInterpolation(false);
    REQUIRE_FALSE(other.needsInterpolation());
}

TEST_CASE("NodeVarying's flag does not follow its interpolation type") {
    // A varying may name an interpolation type and still say it needs none, which is the point
    // of keeping the flag separate: it lets a builder treat the varying as a plain variable.
    const NodeVarying named("v", "float", "smooth");
    REQUIRE(named.interpolationType() == "smooth");
    REQUIRE_FALSE(named.needsInterpolation());
    // And one with no type at all may say it does need interpolating.
    NodeVarying bare("v", "float");
    bare.setNeedsInterpolation(true);
    REQUIRE(bare.interpolationType().empty());
    REQUIRE(bare.needsInterpolation());
}

TEST_CASE("NodeVarying defaults its two interpolation names") {
    // Both start absent, which the port spells as the empty string.
    const NodeVarying varying("v", "float");
    REQUIRE(varying.interpolationType().empty());
    REQUIRE(varying.interpolationSampling().empty());
    // Both may be given.
    const NodeVarying both("v", "float", "smooth", "centroid");
    REQUIRE(both.interpolationType() == "smooth");
    REQUIRE(both.interpolationSampling() == "centroid");
    // One may be given without the other.
    const NodeVarying one("v", "float", "flat");
    REQUIRE(one.interpolationType() == "flat");
    REQUIRE(one.interpolationSampling().empty());
}
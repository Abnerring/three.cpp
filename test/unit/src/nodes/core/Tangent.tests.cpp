#include "nodes/core/Tangent.h"

#include "catch.hpp"

#include <string>

TEST_CASE("Tangent reads a four part attribute") {
    // The fourth part carries which way the frame is wound rather than a direction, so the
    // attribute is four parts wide.
    REQUIRE(std::string(Tangent::kAttributeName) == "tangent");
    REQUIRE(std::string(Tangent::kAttributeType) == "vec4");
    // It is not the three part form the position uses, which is the point of writing it out.
    REQUIRE(std::string(Tangent::kAttributeType) != "vec3");
}

TEST_CASE("Tangent takes its view space value from the attribute when it can") {
    // The vertex stage always has the attribute, and so does any geometry that holds one.
    REQUIRE(Tangent::tangentSource(true, false) == Tangent::Source::Attribute);
    REQUIRE(Tangent::tangentSource(false, true) == Tangent::Source::Attribute);
    REQUIRE(Tangent::tangentSource(true, true) == Tangent::Source::Attribute);
    // Only when neither holds is the frame worked out from derivatives instead.
    REQUIRE(Tangent::tangentSource(false, false) == Tangent::Source::Frame);
}

TEST_CASE("Tangent is lifted as a direction") {
    // A weight of nought stops the translation part of the matrix from moving the tangent.
    REQUIRE(Tangent::kDirectionWeight == 0.0);
    // A weight of one would make it a point, which would be moved, so the two differ.
    REQUIRE(Tangent::kDirectionWeight != 1.0);
}

TEST_CASE("Tangent is turned over unless flat shading is exactly on") {
    // Flat shading being exactly true suppresses the turn, and nothing else does.
    REQUIRE_FALSE(Tangent::negatesOnBackSide(true));
    REQUIRE(Tangent::negatesOnBackSide(false));
    // A builder that answers nothing also turns it over, which the strict comparison gives and
    // a loose one would not; this is the case worth pinning.
    REQUIRE(Tangent::negatesOnBackSide(false));
    REQUIRE(Tangent::negatesOnBackSide(false) != Tangent::negatesOnBackSide(true));
}
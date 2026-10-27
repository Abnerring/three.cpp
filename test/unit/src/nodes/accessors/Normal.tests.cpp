#include "nodes/accessors/Normal.h"

#include "catch.hpp"

#include <string>

TEST_CASE("Normal warns and points up when the attribute is missing") {
    // The message names the attribute and says it was not found.
    REQUIRE(Normal::missingAttributeWarning() ==
            "TSL: Vertex attribute \"normal\" not found on geometry.");
    // The fallback points up, so a surface without a normal reads as facing the sky.
    const auto fallback = Normal::localFallback();
    REQUIRE(fallback.x == 0.0);
    REQUIRE(fallback.y == 1.0);
    REQUIRE(fallback.z == 0.0);
    // It is not the zero vector, which would make every later cross product degenerate.
    REQUIRE(fallback.y != 0.0);
}

TEST_CASE("Normal reads the attribute's name and type") {
    REQUIRE(std::string(Normal::kAttributeName) == "normal");
    REQUIRE(std::string(Normal::kAttributeType) == "vec3");
}

TEST_CASE("Normal's loose test picks the flat normal only when on") {
    // The view space test is loose: on picks the flat normal and off does not.
    REQUIRE(Normal::usesFlatNormalViewGeometry(true));
    REQUIRE_FALSE(Normal::usesFlatNormalViewGeometry(false));
}

TEST_CASE("Normal's strict test adds the varying unless flat shading is exactly on") {
    // The world space test is strict and its answer is the other way round: the varying is
    // added unless flat shading is exactly true.
    REQUIRE_FALSE(Normal::addsWorldGeometryVarying(true));
    REQUIRE(Normal::addsWorldGeometryVarying(false));
    // With flat shading off, the two answers are the two sides of the same coin: no flat normal,
    // and a varying added. Written out so the pair is visible rather than implied.
    REQUIRE_FALSE(Normal::usesFlatNormalViewGeometry(false));
    REQUIRE(Normal::addsWorldGeometryVarying(false));
}

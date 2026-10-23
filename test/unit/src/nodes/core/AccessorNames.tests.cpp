#include "nodes/core/AccessorNames.h"

#include "catch.hpp"

#include <string>

TEST_CASE("AccessorNames builds a uv name from an index") {
    // Index nought gives the plain name with nothing appended.
    REQUIRE(AccessorNames::uvAttribute(0) == "uv");
    // One and up are appended as they are.
    REQUIRE(AccessorNames::uvAttribute(1) == "uv1");
    REQUIRE(AccessorNames::uvAttribute(2) == "uv2");
    REQUIRE(AccessorNames::uvAttribute(10) == "uv10");
}

TEST_CASE("AccessorNames gives the plain name for a negative index") {
    // A negative index fails the same comparison as nought, so it gives the plain name rather
    // than being refused or turned into something like "uv-1".
    REQUIRE(AccessorNames::uvAttribute(-1) == "uv");
    REQUIRE(AccessorNames::uvAttribute(-100) == "uv");
    // The two are the same answer, which is the property rather than an accident of this code.
    REQUIRE(AccessorNames::uvAttribute(-1) == AccessorNames::uvAttribute(0));
    // And the name never carries a minus sign.
    REQUIRE(AccessorNames::uvAttribute(-1).find('-') == std::string::npos);
}

TEST_CASE("AccessorNames names the projection accessor") {
    // The name carries the prefix a varying is shared under, since the accessor is turned into
    // one rather than used as it stands.
    REQUIRE(AccessorNames::modelViewProjectionName() == "v_modelViewProjection");
    REQUIRE(AccessorNames::modelViewProjectionName().find("v_") == 0);
    // The name is not derived from the uv rule, so it never carries a number.
    REQUIRE(AccessorNames::modelViewProjectionName() !=
            AccessorNames::uvAttribute(1));
}
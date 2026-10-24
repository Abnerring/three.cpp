#include "nodes/core/NodeBuilderKinds.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeBuilder turns a colour into three parts and a texture into four") {
    // The answer is about what the value means rather than how many parts it has.
    REQUIRE(NodeBuilderKinds::vectorType("color") == "vec3");
    // All four texture kinds become four parts.
    REQUIRE(NodeBuilderKinds::vectorType("texture") == "vec4");
    REQUIRE(NodeBuilderKinds::vectorType("cubeTexture") == "vec4");
    REQUIRE(NodeBuilderKinds::vectorType("storageTexture") == "vec4");
    REQUIRE(NodeBuilderKinds::vectorType("texture3D") == "vec4");
    // Every other type is itself.
    REQUIRE(NodeBuilderKinds::vectorType("vec2") == "vec2");
    REQUIRE(NodeBuilderKinds::vectorType("float") == "float");
    REQUIRE(NodeBuilderKinds::vectorType("") == "");
    // The colour and the textures differ from one another, so the two numbers are not one rule.
    REQUIRE(NodeBuilderKinds::vectorType("color") != NodeBuilderKinds::vectorType("texture"));
}

TEST_CASE("NodeBuilder looks a typed array's kind up by its class") {
    // Two arrays of the same class answer alike, which a lookup by class gives and a lookup by
    // instance would not.
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("Float32Array") == "float");
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("Float32Array") ==
            NodeBuilderKinds::typeFromArrayClass("Float32Array"));
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("Uint16Array") == "uint");
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("Int8Array") == "int");
    // A class the table does not know answers nothing rather than a default.
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("BigInt64Array").empty());
    REQUIRE(NodeBuilderKinds::typeFromArrayClass("").empty());
}

TEST_CASE("NodeBuilder's integer test searches rather than compares") {
    // The types that really are integers match.
    REQUIRE(NodeBuilderKinds::isInteger("int"));
    REQUIRE(NodeBuilderKinds::isInteger("uint"));
    REQUIRE(NodeBuilderKinds::isInteger("ivec3"));
    REQUIRE(NodeBuilderKinds::isInteger("uvec2"));
    // The types that are not do not.
    REQUIRE_FALSE(NodeBuilderKinds::isInteger("float"));
    REQUIRE_FALSE(NodeBuilderKinds::isInteger("vec3"));
    REQUIRE_FALSE(NodeBuilderKinds::isInteger(""));
    // But a name that merely contains one of the alternatives matches, because the pattern has no
    // anchors: print and joint both count as integers here although neither is a type at all.
    REQUIRE(NodeBuilderKinds::isInteger("print"));
    REQUIRE(NodeBuilderKinds::isInteger("joint"));
    REQUIRE(NodeBuilderKinds::isInteger("int") == NodeBuilderKinds::isInteger("print"));
}

TEST_CASE("NodeBuilder's integer test says no to a boolean vector") {
    // The boolean prefix is not one of the alternatives, so a boolean vector is not an integer
    // even though it is a vector of a scalar kind.
    REQUIRE_FALSE(NodeBuilderKinds::isInteger("bvec2"));
    REQUIRE_FALSE(NodeBuilderKinds::booleanVectorIsInteger());
}
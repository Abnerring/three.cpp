#include "nodes/core/NodeBuilderTypes.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeBuilder knows how many scalars a type holds") {
    // A vector says so in its name.
    REQUIRE(NodeBuilderTypes::typeLength("vec2") == 2);
    REQUIRE(NodeBuilderTypes::typeLength("vec3") == 3);
    REQUIRE(NodeBuilderTypes::typeLength("vec4") == 4);
    // The plain scalars are one.
    REQUIRE(NodeBuilderTypes::typeLength("float") == 1);
    REQUIRE(NodeBuilderTypes::typeLength("bool") == 1);
    REQUIRE(NodeBuilderTypes::typeLength("int") == 1);
    REQUIRE(NodeBuilderTypes::typeLength("uint") == 1);
    // A matrix is the square of its size.
    REQUIRE(NodeBuilderTypes::typeLength("mat2") == 4);
    REQUIRE(NodeBuilderTypes::typeLength("mat3") == 9);
    REQUIRE(NodeBuilderTypes::typeLength("mat4") == 16);
    // And a type that is none of those answers nought, which is neither one nor a failure.
    REQUIRE(NodeBuilderTypes::typeLength("something") == 0);
    REQUIRE(NodeBuilderTypes::typeLength("") == 0);
    REQUIRE(NodeBuilderTypes::typeLength("something") != 1);
}

TEST_CASE("NodeBuilder knows what a type is made of") {
    // A plain type is its own kind.
    REQUIRE(NodeBuilderTypes::componentType("float") == "float");
    REQUIRE(NodeBuilderTypes::componentType("bool") == "bool");
    // A vector or matrix answers the kind its prefix names, with no prefix meaning a float.
    REQUIRE(NodeBuilderTypes::componentType("vec3") == "float");
    REQUIRE(NodeBuilderTypes::componentType("ivec2") == "int");
    REQUIRE(NodeBuilderTypes::componentType("uvec4") == "uint");
    REQUIRE(NodeBuilderTypes::componentType("bvec2") == "bool");
    // And a type that is neither answers nothing rather than a default.
    REQUIRE(NodeBuilderTypes::componentType("something").empty());
}

TEST_CASE("NodeBuilder's two by two matrix collides with a four part vector") {
    // Both hold four scalars, so the length alone cannot say which is meant and the matrix test
    // has to come first.
    REQUIRE(NodeBuilderTypes::typeLength("mat2") == NodeBuilderTypes::typeLength("vec4"));
    REQUIRE(NodeBuilderTypes::isMatrix("mat2"));
    REQUIRE_FALSE(NodeBuilderTypes::isMatrix("vec4"));
    // A matrix of a given length is spelled with mat rather than vec. The prefix rule is not
    // suspended for a matrix kind though, so the first character of the kind is put in front as
    // well and the answer for a matrix kind is a doubled m: mmat4 rather than mat4. That is what
    // the source produces, so the test records it rather than repairing it, and a reader expecting
    // mat4 would be wrong about the port as well as about the source.
    REQUIRE(NodeBuilderTypes::typeFromLength(4, "mat2") == "mmat4");
    REQUIRE(NodeBuilderTypes::typeFromLength(4, "mat2") != "mat4");
    REQUIRE(NodeBuilderTypes::typeFromLength(4, "mat2")[0] == 'm');
    // The same rule applies to any kind that is not a float, so a caller who passed a vector as the
    // kind gets a doubled v: the function expects a scalar kind, and a vector kind is not one. The
    // call is meaningless rather than guarded, and the answer shows it.
    REQUIRE(NodeBuilderTypes::typeFromLength(4, "vec4") == "vvec4");
    REQUIRE(NodeBuilderTypes::typeFromLength(4, "vec4") != "vec4");
    REQUIRE(NodeBuilderTypes::typeFromLength(3, "int") == "ivec3");
    // A length of one is the kind itself, with no vector around it.
    REQUIRE(NodeBuilderTypes::typeFromLength(1, "float") == "float");
    REQUIRE(NodeBuilderTypes::typeFromLength(1, "uint") == "uint");
}

TEST_CASE("NodeBuilder leaves a scalar kind unknown in two cases") {
    // A half float or a normalised attribute does not describe its values as they are seen, so
    // the kind is left unknown rather than guessed.
    REQUIRE(NodeBuilderTypes::scalarKindIsReadable(false, false));
    REQUIRE_FALSE(NodeBuilderTypes::scalarKindIsReadable(true, false));
    REQUIRE_FALSE(NodeBuilderTypes::scalarKindIsReadable(false, true));
    REQUIRE_FALSE(NodeBuilderTypes::scalarKindIsReadable(true, true));
}

TEST_CASE("NodeBuilder answers a missing geometry rather than failing") {
    // Asking before a geometry is attached gives a negative answer rather than an exception.
    REQUIRE(NodeBuilderTypes::hasAttribute(true, true));
    REQUIRE_FALSE(NodeBuilderTypes::hasAttribute(true, false));
    REQUIRE_FALSE(NodeBuilderTypes::hasAttribute(false, false));
    REQUIRE_FALSE(NodeBuilderTypes::hasAttribute(false, true));
}
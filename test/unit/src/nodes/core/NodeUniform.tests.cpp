#include "nodes/core/NodeUniform.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeUniform carries a name and a type") {
    const NodeUniform uniform("lightColour", "vec3");
    REQUIRE(uniform.name() == "lightColour");
    REQUIRE(uniform.type() == "vec3");
    // The flag the source sets for type testing.
    REQUIRE(NodeUniform::isNodeUniform);
}

TEST_CASE("NodeUniform forwards a value it may also write") {
    NodeUniform uniform("u", "float");
    // A fresh uniform starts at zero, since the node's value is what it reports.
    REQUIRE(uniform.value() == 0.0);
    // Writing through the uniform changes the value it reports.
    uniform.setValue(2.5);
    REQUIRE(uniform.value() == 2.5);
    uniform.setValue(-1.0);
    REQUIRE(uniform.value() == -1.0);
}

TEST_CASE("NodeUniform reports an id and a group it cannot change") {
    // The id and the group are given when the uniform is made and only read afterwards, which
    // is the asymmetry against the value that the source's getters and setter express.
    const NodeUniform uniform("u", "float", 42, "frame");
    REQUIRE(uniform.id() == 42);
    REQUIRE(uniform.groupName() == "frame");
    // The two are readable and the interface offers no way to write them, so this comparison
    // stands for what the type system now prevents.
    const NodeUniform other("v", "float", 42, "frame");
    REQUIRE(other.id() == uniform.id());
    REQUIRE(other.groupName() == uniform.groupName());
}

TEST_CASE("NodeUniform defaults its id and group") {
    // A uniform made without the two answers nought and the empty name rather than inventing
    // any.
    const NodeUniform uniform("u", "int");
    REQUIRE(uniform.id() == 0);
    REQUIRE(uniform.groupName().empty());
    REQUIRE(uniform.value() == 0.0);
}
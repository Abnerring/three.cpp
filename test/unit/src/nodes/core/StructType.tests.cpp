#include "nodes/StructType.h"

#include "catch.hpp"

#include <string>

TEST_CASE("StructType holds a name and its members") {
    const StructType type("Light", {{"direction", "vec3"}, {"colour", "vec3"}});
    REQUIRE(type.name() == "Light");
    REQUIRE(type.memberCount() == 2);
    REQUIRE(type.members()[0].first == "direction");
    REQUIRE(type.members()[0].second == "vec3");
    REQUIRE(type.members()[1].first == "colour");
}

TEST_CASE("StructType starts as an input") {
    // The source's constructor sets the flag clear, so a fresh type describes an input.
    StructType type("S", {});
    REQUIRE_FALSE(type.output());
    type.setOutput(true);
    REQUIRE(type.output());
    type.setOutput(false);
    REQUIRE_FALSE(type.output());
}

TEST_CASE("StructType finds a member by name") {
    const StructType type("S", {{"a", "float"}, {"b", "int"}});
    REQUIRE(type.member("a") != nullptr);
    REQUIRE(type.member("a")->second == "float");
    REQUIRE(type.member("b")->second == "int");
    // A name it does not hold gives nothing rather than the last member.
    REQUIRE(type.member("c") == nullptr);
    REQUIRE(type.member("") == nullptr);
    // The lookup is exact, so a differently cased name is not found.
    REQUIRE(type.member("A") == nullptr);
}

TEST_CASE("StructType may be empty") {
    // A type with no members is allowed and simply has none.
    const StructType type("Empty", {});
    REQUIRE(type.name() == "Empty");
    REQUIRE(type.memberCount() == 0);
    REQUIRE(type.members().empty());
    REQUIRE(type.member("a") == nullptr);
}
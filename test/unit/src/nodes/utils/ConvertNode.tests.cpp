#include "nodes/utils/ConvertNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ConvertNode counts the parts of a type") {
    // A float and its neighbours count one; a vector counts its width.
    REQUIRE(ConvertNode::typeLength("float") == 1);
    REQUIRE(ConvertNode::typeLength("int") == 1);
    REQUIRE(ConvertNode::typeLength("vec2") == 2);
    REQUIRE(ConvertNode::typeLength("vec3") == 3);
    REQUIRE(ConvertNode::typeLength("vec4") == 4);
    // A square matrix counts its rows times its columns.
    REQUIRE(ConvertNode::typeLength("mat3") == 9);
    REQUIRE(ConvertNode::typeLength("mat2") == 4);
    REQUIRE(std::string(ConvertNode::kTypeName) == "ConvertNode");
}

TEST_CASE("ConvertNode splits a type list on its bars") {
    const auto one = ConvertNode::parts("vec3");
    REQUIRE(one.size() == 1);
    REQUIRE(one[0] == "vec3");
    const auto three = ConvertNode::parts("vec3|float|int");
    REQUIRE(three.size() == 3);
    REQUIRE(three[0] == "vec3");
    REQUIRE(three[1] == "float");
    REQUIRE(three[2] == "int");
    // The order is kept, which matters because the last match wins.
    REQUIRE(three[2] != three[0]);
}

TEST_CASE("ConvertNode answers with the first part when none matches") {
    // A two part request against a list with no two part type: the first part is kept because
    // it was chosen before any comparison could fail, so the answer is a three part vector
    // rather than nothing.
    REQUIRE(ConvertNode::choose("vec2", "vec3|float|int") == "vec3");
    REQUIRE_FALSE(ConvertNode::choose("vec2", "vec3|float|int").empty());
    // The same with a single part list, whatever the request.
    REQUIRE(ConvertNode::choose("vec4", "float") == "float");
}

TEST_CASE("ConvertNode answers with the last match, not the first") {
    // Three parts of length one in the list and a one part request: every part matches, so the
    // last is kept. A reader would expect the first.
    REQUIRE(ConvertNode::choose("float", "vec3|float|int") == "int");
    REQUIRE(ConvertNode::choose("float", "vec3|float|int") != "float");
    // Two matches behave the same way.
    REQUIRE(ConvertNode::choose("float", "float|int") == "int");
    REQUIRE(ConvertNode::choose("float", "int|float") == "float");
    // The order in the list therefore changes the answer, which is what "last" means.
    REQUIRE(ConvertNode::choose("float", "float|int") != ConvertNode::choose("float", "int|float"));
}

TEST_CASE("ConvertNode's two rules meet when the first part matches") {
    // When the first part already has the right length, later matches can still displace it, so
    // the answer is still the last matching part.
    REQUIRE(ConvertNode::choose("vec3", "vec3|vec3") == "vec3");
    // A list whose only match is the first but which has later non matches keeps the first.
    REQUIRE(ConvertNode::choose("vec2", "vec2|float|int") == "vec2");
    // And a later match replaces it.
    REQUIRE(ConvertNode::choose("vec2", "vec2|vec2|float") == "vec2");
}
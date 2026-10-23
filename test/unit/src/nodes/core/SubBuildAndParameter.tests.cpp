#include "nodes/core/ParameterNode.h"
#include "nodes/core/SubBuildNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SubBuildNode brackets its build always") {
    // The child is always built inside the pair.
    REQUIRE(SubBuildNode::bracketsTheBuild());
    REQUIRE(std::string(SubBuildNode::kTypeName) == "SubBuild");
    REQUIRE(SubBuildNode::isSubBuildNode);
}

TEST_CASE("SubBuildNode skips the bracket when the type was given") {
    // A type supplied at construction is returned before the bracket opens, so a caller with a
    // type causes one fewer pair of builder calls than one without.
    REQUIRE_FALSE(SubBuildNode::bracketsTheTypeQuery(true));
    REQUIRE(SubBuildNode::bracketsTheTypeQuery(false));
    // The two differ, which is the whole consequence of supplying a type.
    REQUIRE(SubBuildNode::bracketsTheTypeQuery(true) != SubBuildNode::bracketsTheTypeQuery(false));
    // And the pair is not protected on the way out, so an exception between them would leave it
    // open.
    REQUIRE_FALSE(SubBuildNode::balancedOnThrow());
    REQUIRE_FALSE(SubBuildNode::isCacheable());
}

TEST_CASE("ParameterNode's missing structure gives a float after an error") {
    // With a structure the question goes to it.
    REQUIRE(ParameterNode::memberType(true, "vec3") == "vec3");
    // Without one the answer is a float, and an error is reported first rather than instead.
    REQUIRE(ParameterNode::memberType(false, "vec3") == "float");
    REQUIRE(ParameterNode::memberType(false, "vec3") != "vec3");
    // That default is the same one the structure family's member node uses, which the two
    // families happen to agree on even though they disagree elsewhere.
    REQUIRE(std::string(ParameterNode::kMissingStructureMemberType) == "float");
    REQUIRE(std::string(ParameterNode::kTypeName) == "ParameterNode");
}

TEST_CASE("ParameterNode's error names both the member and the type") {
    REQUIRE(ParameterNode::missingStructureMessage("colour", "LightStruct") ==
            "TSL: Member \"colour\" not found in struct \"LightStruct\".");
    // Both names are part of the sentence, so two different pairs give two different sentences.
    REQUIRE(ParameterNode::missingStructureMessage("colour", "S") !=
            ParameterNode::missingStructureMessage("size", "S"));
    REQUIRE(ParameterNode::missingStructureMessage("colour", "S") !=
            ParameterNode::missingStructureMessage("colour", "T"));
}

TEST_CASE("ParameterNode's hash is its identifier as text") {
    // The hash is a string, so the text is what a caller compares.
    REQUIRE(ParameterNode::hash(42) == "42");
    REQUIRE(ParameterNode::hash(0) == "0");
    // Different identifiers give different text.
    REQUIRE(ParameterNode::hash(42) != ParameterNode::hash(43));
    // And the same identifier always gives the same text, which is what makes it usable as a key.
    REQUIRE(ParameterNode::hash(7) == ParameterNode::hash(7));
}

TEST_CASE("ParameterNode generates its name without checking it") {
    // A name that was given is produced as it stands.
    REQUIRE(ParameterNode::generate(true, "lightColour") == "lightColour");
    // A name that was not is not caught: the absent value renders as its own text rather than
    // being refused or replaced by something the node supplies.
    REQUIRE(ParameterNode::generate(false, "") == "null");
    REQUIRE(ParameterNode::generate(false, "") != "");
    REQUIRE(ParameterNode::isParameterNode);
}
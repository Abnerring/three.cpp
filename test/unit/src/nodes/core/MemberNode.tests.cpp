#include "nodes/core/MemberNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("MemberNode's missing member is a float") {
    // A name that is not there gives a float rather than an error, so a shader that reads it
    // still compiles and gives nought where the member would have been.
    REQUIRE(MemberNode::memberType(false, "vec3") == "float");
    REQUIRE(MemberNode::memberType(false, "vec3") != "vec3");
    REQUIRE(std::string(MemberNode::kMissingMemberType) == "float");
    // A member that is there keeps its own type.
    REQUIRE(MemberNode::memberType(true, "vec3") == "vec3");
    REQUIRE(MemberNode::memberType(true, "mat4") == "mat4");
    // The two cases differ, which is the point of the test.
    REQUIRE(MemberNode::memberType(true, "vec3") != MemberNode::memberType(false, "vec3"));
}

TEST_CASE("MemberNode's warning names the member") {
    REQUIRE(MemberNode::missingMemberWarning("colour") ==
            "TSL: Member \"colour\" does not exist in struct.");
    // The name really is part of the sentence.
    REQUIRE(MemberNode::missingMemberWarning("colour") != MemberNode::missingMemberWarning("size"));
    REQUIRE(std::string(MemberNode::kTypeName) == "MemberNode");
}

TEST_CASE("MemberNode's existence check runs up the chain") {
    // A plain structure only needs its own member.
    REQUIRE(MemberNode::hasMember(false, false, true));
    REQUIRE_FALSE(MemberNode::hasMember(false, false, false));
    // A structure that is itself a member needs that one first, so a member of a member is only
    // there when the one before it is.
    REQUIRE(MemberNode::hasMember(true, true, true));
    REQUIRE_FALSE(MemberNode::hasMember(true, false, true));
    REQUIRE_FALSE(MemberNode::hasMember(true, false, false));
    // The two levels therefore both have to hold.
    REQUIRE(MemberNode::hasMember(true, true, true) != MemberNode::hasMember(true, false, true));
}

TEST_CASE("MemberNode's type guard asks about the wrong name") {
    // The method that answers a type takes a name and guards on the existence of the node's own
    // property, so asking about a different name is answered under the wrong question. That is
    // what the source does, and a test that assumed the guard followed the argument would be
    // wrong.
    REQUIRE(MemberNode::guardUsesOwnProperty());
    // The consequence is that a missing member can be reported as found when the node's own
    // property happens to exist, which is why the flag is recorded rather than repaired.
    REQUIRE(MemberNode::memberType(MemberNode::guardUsesOwnProperty(), "vec2") == "vec2");
}

TEST_CASE("MemberNode refuses to be cached") {
    REQUIRE_FALSE(MemberNode::isCacheable());
}
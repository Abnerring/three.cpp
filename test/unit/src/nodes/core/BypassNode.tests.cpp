#include "nodes/core/BypassNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("BypassNode reports the output's type") {
    // The node runs a call and answers with the output, so its type is the output's and the
    // call's is never asked for.
    REQUIRE(BypassNode::nodeType("vec3") == "vec3");
    REQUIRE(BypassNode::nodeType("float") == "float");
    // The call's type would be something else, and it does not appear here.
    REQUIRE(BypassNode::nodeType("vec3") != "void");
    REQUIRE(BypassNode::isBypassNode);
}

TEST_CASE("BypassNode builds its call as the empty type") {
    // The call is forced to the empty type, so a call that would otherwise return a value has
    // that value discarded rather than converted or stored.
    REQUIRE(std::string(BypassNode::kCallBuildType) == "void");
    REQUIRE(std::string(BypassNode::kCallBuildType) != "float");
    REQUIRE(std::string(BypassNode::kTypeName) == "BypassNode");
}

TEST_CASE("BypassNode adds a line only for a non empty call") {
    // An empty text adds nothing, so a call whose text comes out empty produces no line at all
    // rather than a blank one.
    REQUIRE_FALSE(BypassNode::addsLine(""));
    REQUIRE(BypassNode::addsLine("discard;"));
    REQUIRE(BypassNode::addsLine(" "));
    // A single space counts as a text, so it does add a line; only the empty string does not.
    REQUIRE(BypassNode::addsLine(" ") != BypassNode::addsLine(""));
}

TEST_CASE("BypassNode builds the call before the output") {
    // The call comes first, so its text is emitted before the output it happens beside.
    REQUIRE(BypassNode::buildsCallFirst());
    REQUIRE_FALSE(BypassNode::isCacheable());
}
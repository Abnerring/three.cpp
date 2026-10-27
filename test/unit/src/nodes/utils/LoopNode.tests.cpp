#include "nodes/utils/LoopNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("LoopNode's variable names are an offset from the letter i") {
    // The first three are the letters a reader would guess.
    REQUIRE(LoopNode::varName(0) == "i");
    REQUIRE(LoopNode::varName(1) == "j");
    REQUIRE(LoopNode::varName(2) == "k");
    // The fourth is l, which comes from the offset rather than from a table of names.
    REQUIRE(LoopNode::varName(3) == "l");
    REQUIRE(LoopNode::varName(4) == "m");
    // Two neighbouring positions never share a name.
    REQUIRE(LoopNode::varName(0) != LoopNode::varName(1));
    REQUIRE(LoopNode::varName(3) != LoopNode::varName(2));
    REQUIRE(std::string(LoopNode::kNodeType) == "void");
}

TEST_CASE("LoopNode turns a bare callback into an unbounded loop") {
    // A function where a bound would be has no number to count towards, so a constant true is put
    // in front of it and the callback becomes the body.
    const auto bare = LoopNode::internalParams(true);
    REQUIRE(bare.size() == 2);
    REQUIRE(bare[0] == "bool(true)");
    // A list of parameters is taken as it stands.
    const auto given = LoopNode::internalParams(false);
    REQUIRE(given[0] != "bool(true)");
    REQUIRE(LoopNode::internalParams(true)[0] != LoopNode::internalParams(false)[0]);
    // And in the bare case there are no descriptors, since the one real entry is the body.
    REQUIRE(LoopNode::descriptorCount(1) == 0);
}

TEST_CASE("LoopNode treats every parameter but the last as a descriptor") {
    // A one entry list has none, a two entry one has one, and so on; the last is always the body.
    REQUIRE(LoopNode::descriptorCount(1) == 0);
    REQUIRE(LoopNode::descriptorCount(2) == 1);
    REQUIRE(LoopNode::descriptorCount(3) == 2);
    REQUIRE(LoopNode::descriptorCount(0) == 0);
    REQUIRE(LoopNode::descriptorCount(3) != LoopNode::descriptorCount(2));
    REQUIRE(std::string(LoopNode::kTypeName) == "LoopNode");
}

TEST_CASE("LoopNode defaults a descriptor's name and type") {
    // A descriptor carrying its own name keeps it, and one without gets the positional name.
    REQUIRE(LoopNode::paramName(false, true, "count", 0) == "count");
    REQUIRE(LoopNode::paramName(false, false, "count", 0) == "i");
    REQUIRE(LoopNode::paramName(false, false, "count", 1) == "j");
    // A node is never asked for a name or a type, so it falls back in both cases.
    REQUIRE(LoopNode::paramName(true, true, "count", 2) == "k");
    // The type defaults to an integer rather than to nothing.
    REQUIRE(LoopNode::paramType(false, false, "float") == "int");
    REQUIRE(LoopNode::paramType(false, true, "float") == "float");
    REQUIRE(std::string(LoopNode::kDefaultType) == "int");
    // The body runs once, since invoking it twice would run its side effects twice.
    REQUIRE(LoopNode::bodyIsInvokedOnce());
    REQUIRE_FALSE(LoopNode::isCacheable());
}
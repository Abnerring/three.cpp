#include "nodes/core/StructTypeNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

using Member = StructTypeNode::Member;

TEST_CASE("StructTypeNode knows each type's size and alignment") {
    // A float takes one place and starts anywhere.
    REQUIRE(StructTypeNode::memoryLength("float") == 1);
    REQUIRE(StructTypeNode::alignment("float") == 1);
    // A three part vector takes three places but must start on a multiple of four, which is the
    // whole reason for the padding.
    REQUIRE(StructTypeNode::memoryLength("vec3") == 3);
    REQUIRE(StructTypeNode::alignment("vec3") == 4);
    REQUIRE(StructTypeNode::alignment("vec3") != StructTypeNode::memoryLength("vec3"));
    // A four part vector takes and needs four.
    REQUIRE(StructTypeNode::memoryLength("vec4") == 4);
    REQUIRE(StructTypeNode::alignment("vec4") == 4);
    REQUIRE(std::string(StructTypeNode::kConstructorType) == "struct");
    REQUIRE(StructTypeNode::isStructTypeNode);
}

TEST_CASE("StructTypeNode rounds a single member up to its alignment") {
    // A float alone needs one place and rounds to one.
    REQUIRE(StructTypeNode::length({{1, 1}}) == 1);
    // A three part vector alone needs three but rounds to four.
    REQUIRE(StructTypeNode::length({{3, 4}}) == 4);
    REQUIRE(StructTypeNode::length({{3, 4}}) != 3);
    // An empty structure needs nothing.
    REQUIRE(StructTypeNode::length({}) == 0);
}

TEST_CASE("StructTypeNode pads a second member to its own alignment") {
    // Two three part vectors: the first takes three, the second is moved to four and takes three,
    // and the total of seven rounds to eight.
    REQUIRE(StructTypeNode::length({{3, 4}, {3, 4}}) == 8);
    REQUIRE(StructTypeNode::length({{3, 4}, {3, 4}}) != 6);
    // A four part vector followed by a float: the float starts at four, taking the total to five,
    // which rounds to eight because the largest alignment is four.
    REQUIRE(StructTypeNode::length({{4, 4}, {1, 1}}) == 8);
    REQUIRE(StructTypeNode::length({{4, 4}, {1, 1}}) != 5);
    // A float followed by a float needs two places and rounds to two.
    REQUIRE(StructTypeNode::length({{1, 1}, {1, 1}}) == 2);
}

TEST_CASE("StructTypeNode's largest alignment is updated before the padding") {
    // A member own alignment counts towards the chunk it is padded in, because the maximum is
    // updated first. Two parts of two and one show the difference: without the early update the
    // first two would be padded in a chunk of one. Here the walk reaches three and the last
    // step rounds that up to a multiple of the largest alignment, which is two, so the
    // answer is four; working the walk out without that last step gives three and is wrong.
    REQUIRE(StructTypeNode::length({{2, 2}, {1, 1}}) == 4);
    REQUIRE(StructTypeNode::length({{2, 2}, {1, 1}}) != 3);
    // And the total is rounded to the largest alignment, which here is two.
    REQUIRE(StructTypeNode::length({{2, 2}, {2, 2}}) == 4);
    REQUIRE(StructTypeNode::length({{2, 2}, {2, 2}}) % 2 == 0);
}

TEST_CASE("StructTypeNode's missing member is the empty type") {
    // A name that is there gives its type.
    REQUIRE(StructTypeNode::memberType({"a", "b"}, {"vec3", "float"}, "a") == "vec3");
    REQUIRE(StructTypeNode::memberType({"a", "b"}, {"vec3", "float"}, "b") == "float");
    // One that is not gives the empty type, which is what the storage element node answers and
    // the opposite of what the member, parameter and attribute nodes answer.
    REQUIRE(StructTypeNode::memberType({"a"}, {"vec3"}, "c") == "void");
    REQUIRE(StructTypeNode::memberType({"a"}, {"vec3"}, "c") != "float");
    REQUIRE(std::string(StructTypeNode::kMissingMemberType) == "void");
}

TEST_CASE("StructTypeNode's atomic flag defaults to not atomic") {
    // A member given as a bare string is not atomic.
    REQUIRE_FALSE(StructTypeNode::atomicForBareString());
    // One given as an object takes its flag, falling back when the flag is falsy.
    REQUIRE(StructTypeNode::atomicForObject(true, true));
    REQUIRE_FALSE(StructTypeNode::atomicForObject(true, false));
    REQUIRE_FALSE(StructTypeNode::atomicForObject(false, false));
    // The bare string's answer agrees with the object's default rather than its set case.
    REQUIRE(StructTypeNode::atomicForBareString() == StructTypeNode::atomicForObject(false, false));
    REQUIRE(std::string(StructTypeNode::kTypeName) == "StructTypeNode");
}

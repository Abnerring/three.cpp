#include "nodes/utils/StorageArrayElementNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("StorageArrayElementNode's missing member is the empty type") {
    // With a struct type the question goes to it.
    REQUIRE(StorageArrayElementNode::memberType(true, "vec3") == "vec3");
    // Without one the answer is the empty type, which is not a type a value has.
    REQUIRE(StorageArrayElementNode::memberType(false, "vec3") == "void");
    REQUIRE(StorageArrayElementNode::memberType(false, "vec3") != "vec3");
    REQUIRE(std::string(StorageArrayElementNode::kMissingMemberType) == "void");
    REQUIRE(std::string(StorageArrayElementNode::kTypeName) == "StorageArrayElementNode");
}

TEST_CASE("StorageArrayElementNode disagrees with the member node about a missing member") {
    // The structure family's member node gives a float in the same position, so the two
    // families answer the same question differently.
    REQUIRE(std::string(StorageArrayElementNode::kMemberNodeMissingType) == "float");
    REQUIRE_FALSE(StorageArrayElementNode::familiesAgreeOnMissingMember());
    // The two answers really are different, which is what makes the disagreement worth noting.
    REQUIRE(std::string(StorageArrayElementNode::kMissingMemberType) !=
            std::string(StorageArrayElementNode::kMemberNodeMissingType));
}

TEST_CASE("StorageArrayElementNode does not format an assignment") {
    // An assignment needs the thing being written, so the conversion formatting is left out.
    REQUIRE_FALSE(StorageArrayElementNode::formats(true));
    // A read is formatted as usual.
    REQUIRE(StorageArrayElementNode::formats(false));
    REQUIRE(StorageArrayElementNode::formats(true) != StorageArrayElementNode::formats(false));
}

TEST_CASE("StorageArrayElementNode falls back only in a particular mix") {
    // With the buffer available the storage path is used whatever else is true.
    REQUIRE_FALSE(StorageArrayElementNode::usesPboFallback(true, true, false, true, false));
    // Without it, the fallback needs the node to have asked for it.
    REQUIRE_FALSE(StorageArrayElementNode::usesPboFallback(false, false, false, true, false));
    // And it is not used in an assignment.
    REQUIRE_FALSE(StorageArrayElementNode::usesPboFallback(false, true, true, true, false));
    // With the first three satisfied, either the value being an instanced attribute or the stage
    // not being a compute one is enough.
    REQUIRE(StorageArrayElementNode::usesPboFallback(false, true, false, true, true));
    REQUIRE(StorageArrayElementNode::usesPboFallback(false, true, false, false, false));
    // Neither of those is not enough.
    REQUIRE_FALSE(StorageArrayElementNode::usesPboFallback(false, true, false, false, true));
}
#include "nodes/core/NodeAccessors.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

TEST_CASE("Node's member type default is the empty type, not a float") {
    // The base class answers with the empty type, and three of its subclasses replace that with a
    // float while two others keep it; so the empty type is the rule and the float the exception.
    REQUIRE(std::string(NodeAccessors::kMemberTypeDefault) == "void");
    REQUIRE(std::string(NodeAccessors::kMemberTypeDefault) != "float");
    REQUIRE(std::string(NodeAccessors::kMemberTypeDefault) != "int");
}

TEST_CASE("Node's scope is the node itself") {
    // Not a name and not a lookup: the scope by default is the node.
    REQUIRE(NodeAccessors::scopeIsTheNodeItself());
}

TEST_CASE("Node's hash is its identifier as text and its custom key as a number") {
    // Two answers for the same identifier, one a string and one a number, because two different
    // questions are being answered.
    REQUIRE(NodeAccessors::hash(42) == "42");
    REQUIRE(NodeAccessors::customCacheKey(42) == 42);
    // The text and the number are not interchangeable, which the two return types make plain.
    REQUIRE(NodeAccessors::hash(42) != NodeAccessors::hash(43));
    REQUIRE(NodeAccessors::customCacheKey(42) != NodeAccessors::customCacheKey(43));
    REQUIRE(NodeAccessors::hash(0) == "0");
}

TEST_CASE("Node strips four characters from a stored child name") {
    // Children are held under the names of the properties that own them, and those names end in
    // the word for a node, so removing it leaves the property's own name.
    REQUIRE(NodeAccessors::propertyNameFromStorageName("colourNode") == "colour");
    REQUIRE(NodeAccessors::propertyNameFromStorageName("aNode") == "a");
    // A name too short to strip gives nothing rather than a negative length.
    REQUIRE(NodeAccessors::propertyNameFromStorageName("Node").empty());
    REQUIRE(NodeAccessors::propertyNameFromStorageName("ab").empty());
    // The stripped answer is four characters shorter, which is the whole rule.
    REQUIRE(NodeAccessors::propertyNameFromStorageName("xyNode").size() == 2);
}

TEST_CASE("Node recomputes a cache key when asked or when its version moved") {
    // Either condition is enough, so the two are not distinguishable afterwards.
    REQUIRE(NodeAccessors::shouldRecompute(true, 0, 0));
    REQUIRE(NodeAccessors::shouldRecompute(false, 1, 0));
    REQUIRE(NodeAccessors::shouldRecompute(true, 1, 0));
    // Only when neither holds is the key kept.
    REQUIRE_FALSE(NodeAccessors::shouldRecompute(false, 0, 0));
    REQUIRE_FALSE(NodeAccessors::shouldRecompute(false, 3, 3));
    REQUIRE(NodeAccessors::shouldRecompute(false, 1, 0) !=
            NodeAccessors::shouldRecompute(false, 0, 0));
}
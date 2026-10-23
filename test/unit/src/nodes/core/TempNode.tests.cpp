#include "nodes/core/TempNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("TempNode says it is deprecated") {
    // The module asks to be replaced, and says so in the same words upstream uses.
    REQUIRE(TempNode::deprecationWarning() ==
            "TempNode: This module has been deprecated. Extend Node instead.");
    // The name and the flag the source sets.
    REQUIRE(std::string(TempNode::kTypeName) == "TempNode");
    REQUIRE(TempNode::isTempNode);
}

TEST_CASE("TempNode caches only a node used more than once") {
    // A node used exactly once is not given a temporary variable, since working it out a second
    // time never happens.
    REQUIRE_FALSE(TempNode::isCacheable(0));
    REQUIRE_FALSE(TempNode::isCacheable(1));
    // Two uses and up are cached.
    REQUIRE(TempNode::isCacheable(2));
    REQUIRE(TempNode::isCacheable(3));
    REQUIRE(TempNode::isCacheable(100));
}

TEST_CASE("TempNode's two questions are the same one") {
    // The caching rule asks whether there is more than one dependency, so the two answers never
    // differ, which a sweep shows rather than a remark in a comment.
    for (std::uint32_t count = 0; count < 20; ++count) {
        REQUIRE(TempNode::isCacheable(count) == TempNode::hasDependencies(count));
    }
    // The boundary is between one and two, not between nought and one.
    REQUIRE_FALSE(TempNode::hasDependencies(1));
    REQUIRE(TempNode::hasDependencies(2));
}
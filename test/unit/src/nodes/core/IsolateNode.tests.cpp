#include "nodes/core/IsolateNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("IsolateNode swaps the cache in three steps") {
    // The old cache is remembered, this node's is set, and the old one is put back.
    const auto steps = IsolateNode::swapSteps();
    REQUIRE(steps.size() == 3);
    REQUIRE(steps[0] == "getCache");
    REQUIRE(steps[1] == "setCache");
    REQUIRE(steps[2] == "restoreCache");
    // The restore is the last step, which is what makes it a swap rather than a set.
    REQUIRE(steps[2] != steps[1]);
    REQUIRE(IsolateNode::kDefaultParent);
    REQUIRE(IsolateNode::isIsolateNode);
}

TEST_CASE("IsolateNode does not put the cache back when the work throws") {
    // The restore is a plain statement after the call, so an exception leaves the builder
    // pointed at this node's cache. This is the same shape as the sub build's bracket.
    REQUIRE_FALSE(IsolateNode::restoresOnThrow());
    // The type query swaps the cache too, so asking a child for its type is not read-only.
    REQUIRE(IsolateNode::typeQuerySwapsCache());
    REQUIRE(IsolateNode::typeQuerySwapsCache() != IsolateNode::restoresOnThrow());
    REQUIRE(std::string(IsolateNode::kTypeName) == "IsolateNode");
}

TEST_CASE("IsolateNode's deprecated name still works") {
    // The longer form is deprecated, prints a warning, and then makes the shorter one and sets
    // the parent afterwards, since the shorter one has no parameter for it.
    REQUIRE(IsolateNode::deprecatedFormSetsParentAfterwards());
    REQUIRE(IsolateNode::bothNamesChain());
    // The warning names both the old name and the one to use instead.
    const std::string warning = IsolateNode::deprecationWarning();
    REQUIRE(warning.find("cache()") != std::string::npos);
    REQUIRE(warning.find("isolate()") != std::string::npos);
    REQUIRE(warning.find("deprecated") != std::string::npos);
    REQUIRE(warning == "TSL: \"cache()\" has been deprecated. Use \"isolate()\" instead.");
}
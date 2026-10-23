#include "nodes/core/RTTNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("RTTNode resizes itself exactly when no width was given") {
    // A node made without a width resizes itself as the frame goes on.
    REQUIRE(RTTNode::autoResize(false));
    // One made with a width does not, since the caller has said what size it wants.
    REQUIRE_FALSE(RTTNode::autoResize(true));
    // So the two differ, and there is no flag to set: the answer follows the width.
    REQUIRE(RTTNode::autoResize(false) != RTTNode::autoResize(true));
    REQUIRE(RTTNode::isRTTNode);
    REQUIRE(std::string(RTTNode::kTypeName) == "RTTNode");
}

TEST_CASE("RTTNode's default size is one and does not swallow nought") {
    // With no size at all the default is one, which is smaller than any sensible picture; the
    // target is meant to be resized later.
    REQUIRE(RTTNode::sizeOr(0, false) == 1);
    REQUIRE(RTTNode::sizeOr(512, false) == 1);
    REQUIRE(RTTNode::kDefaultSize == 1);
    // A size of nought is kept, because the test is for a missing size rather than for a falsy
    // one; a caller who asks for nothing gets nothing.
    REQUIRE(RTTNode::sizeOr(0, true) == 0);
    REQUIRE(RTTNode::sizeOr(0, true) != RTTNode::sizeOr(0, false));
    // And a real size is kept as given.
    REQUIRE(RTTNode::sizeOr(512, true) == 512);
}

TEST_CASE("RTTNode lets the caller override the texture type") {
    // Without a named type the default is a half float, which is what a render meant to be read
    // back or blended wants.
    REQUIRE(RTTNode::textureType(false, "UnsignedByteType") == "HalfFloatType");
    // A caller who names one gets it, because the option list is spread over the default.
    REQUIRE(RTTNode::textureType(true, "UnsignedByteType") == "UnsignedByteType");
    REQUIRE(RTTNode::textureType(true, "UnsignedByteType") !=
            RTTNode::textureType(false, "UnsignedByteType"));
    REQUIRE(std::string(RTTNode::kTypeName) == "RTTNode");
}

TEST_CASE("RTTNode keeps its renderer state to itself") {
    // The state is kept per node rather than shared, so a node rendered from inside another's
    // render cannot overwrite the outer one's.
    REQUIRE(RTTNode::stateIsPerInstance());
    // The refresh is once per frame.
    REQUIRE(std::string(RTTNode::kUpdateType) == "FRAME");
    REQUIRE_FALSE(RTTNode::isCacheable());
}
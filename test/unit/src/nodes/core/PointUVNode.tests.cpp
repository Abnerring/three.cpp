#include "nodes/core/PointUVNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("PointUVNode names itself and its type") {
    REQUIRE(std::string(PointUVNode::kTypeName) == "PointUVNode");
    REQUIRE(std::string(PointUVNode::kNodeType) == "vec2");
    REQUIRE(PointUVNode::isPointUVNode);
}

TEST_CASE("PointUVNode is never cacheable") {
    // The answer is a plain no, unlike the node that decides from how many times it is used.
    REQUIRE_FALSE(PointUVNode::isCacheable());
}

TEST_CASE("PointUVNode turns the vertical coordinate over") {
    // The horizontal coordinate is passed through and the vertical one is taken away from one,
    // since the point coordinate counts downwards and the image upwards.
    const std::string text = PointUVNode::generate();
    REQUIRE(text == "vec2( gl_PointCoord.x, 1.0 - gl_PointCoord.y )");
    // The horizontal is not turned over; only one of the two is.
    REQUIRE(text.find("1.0 - gl_PointCoord.x") == std::string::npos);
    REQUIRE(text.find("1.0 - gl_PointCoord.y") != std::string::npos);
    // The result is a two part vector, which the type already said.
    REQUIRE(text.find("vec2") == 0);
}
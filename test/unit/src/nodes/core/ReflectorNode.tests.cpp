#include "nodes/core/ReflectorNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ReflectorNode skips a quad mesh") {
    // The reflector is built for ordinary objects.
    REQUIRE(ReflectorNode::buildsFor(false));
    // A quad mesh is skipped, which keeps the node out of post processing where every object is a
    // quad and a reflection would mean nothing.
    REQUIRE_FALSE(ReflectorNode::buildsFor(true));
    REQUIRE(ReflectorNode::buildsFor(false) != ReflectorNode::buildsFor(true));
    REQUIRE(std::string(ReflectorNode::kTypeName) == "ReflectorNode");
}

TEST_CASE("ReflectorNode refuses a depth node unless depth was asked for") {
    // The option has to be on.
    REQUIRE(ReflectorNode::mayAskForDepth(true));
    REQUIRE_FALSE(ReflectorNode::mayAskForDepth(false));
    // And the refusal is a throw whose sentence ends with a space before the closing quote, which
    // is part of the text rather than an accident of this port.
    const std::string message = ReflectorNode::depthNotEnabledMessage();
    REQUIRE(message.find("depth: true") != std::string::npos);
    REQUIRE(message.back() == ' ');
    REQUIRE(message.size() > 0);
    REQUIRE(message.back() != '.');
}

TEST_CASE("ReflectorNode turns off its texture matrix") {
    // The reflection is sampled in screen space, so a matrix would be the wrong thing to apply;
    // the node turns the update off when it is made.
    REQUIRE_FALSE(ReflectorNode::updatesMatrix());
}

TEST_CASE("ReflectorNode shares one rendering flag across every instance") {
    // While one reflector is being rendered a second is declined, so a mirror cannot show itself.
    REQUIRE(ReflectorNode::skipsWhileOneIsBeingRendered(true));
    REQUIRE_FALSE(ReflectorNode::skipsWhileOneIsBeingRendered(false));
    // The defaults a caller who says nothing gets.
    REQUIRE_FALSE(ReflectorNode::kDefaultGenerateMipmaps);
    REQUIRE(ReflectorNode::kDefaultBounces);
    REQUIRE_FALSE(ReflectorNode::kDefaultDepth);
    REQUIRE(ReflectorNode::kDefaultResolutionScale == 1);
}
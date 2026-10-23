#include "nodes/core/UniformGroupNode.h"

#include "catch.hpp"

#include <string>

using Preset = UniformGroupNode::Preset;

TEST_CASE("UniformGroupNode reports the word for a string") {
    // The type says nothing about uniforms, which is what makes it worth recording.
    REQUIRE(std::string(UniformGroupNode::kNodeType) == "string");
    REQUIRE(std::string(UniformGroupNode::kNodeType) != "vec4");
    REQUIRE(UniformGroupNode::isUniformGroup);
    REQUIRE(std::string(UniformGroupNode::kTypeName) == "UniformGroupNode");
}

TEST_CASE("UniformGroupNode's two factories have different default orders") {
    // The plain one starts at one and the shared one at nought, so shared groups sort first
    // without a caller saying so.
    REQUIRE(UniformGroupNode::defaultOrder(false) == 1);
    REQUIRE(UniformGroupNode::defaultOrder(true) == 0);
    REQUIRE(UniformGroupNode::kPlainOrder == 1);
    REQUIRE(UniformGroupNode::kSharedOrder == 0);
    // The two differ, which is the whole point of having two defaults.
    REQUIRE(UniformGroupNode::defaultOrder(false) != UniformGroupNode::defaultOrder(true));
}

TEST_CASE("UniformGroupNode's three presets differ in the right ways") {
    // The frame and render groups are both shared and at the shared order.
    const Preset frame = UniformGroupNode::frameGroup();
    const Preset render = UniformGroupNode::renderGroup();
    REQUIRE(frame.name == "frame");
    REQUIRE(frame.shared);
    REQUIRE(frame.order == 0);
    REQUIRE(frame.updateType == "FRAME");
    REQUIRE(render.name == "render");
    REQUIRE(render.shared);
    REQUIRE(render.order == 0);
    REQUIRE(render.updateType == "RENDER");
    // The object group is the odd one: not shared, at the plain order, and updated per object.
    const Preset object = UniformGroupNode::objectGroup();
    REQUIRE(object.name == "object");
    REQUIRE_FALSE(object.shared);
    REQUIRE(object.order == 1);
    REQUIRE(object.updateType == "OBJECT");
    // So its order differs from the other two, which is the consequence of not being shared.
    REQUIRE(object.order != frame.order);
}

TEST_CASE("UniformGroupNode leaves two of its settings out of the serialised form") {
    // The name, the version and the shared flag are written; the order and the update kind are
    // not, so a group read back has a fresh order and no update kind.
    REQUIRE_FALSE(UniformGroupNode::serialisesOrder());
    REQUIRE_FALSE(UniformGroupNode::serialisesUpdateType());
    // The round trip is therefore lossy for two of the five settings.
    REQUIRE(UniformGroupNode::serialisesOrder() == UniformGroupNode::serialisesUpdateType());
}

TEST_CASE("UniformGroupNode sets a flag it never initialises") {
    // Updating sets a flag saying an update is needed.
    REQUIRE(UniformGroupNode::setsNeedsUpdateFlag());
    // Nothing gives that flag a starting value, so before the first update it is absent rather
    // than false, which is a different thing to test.
    REQUIRE_FALSE(UniformGroupNode::initialisesNeedsUpdate());
    REQUIRE(UniformGroupNode::setsNeedsUpdateFlag() != UniformGroupNode::initialisesNeedsUpdate());
    REQUIRE_FALSE(UniformGroupNode::isCacheable());
}
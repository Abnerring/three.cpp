#include "nodes/accessors/UserDataNode.h"
#include "catch.hpp"

TEST_CASE("UserDataNode exposes the pinned constructor and factory") {
    const auto ctor = UserDataNode::constructor();
    REQUIRE(ctor.isConstructible());
    REQUIRE(ctor.get("type").toString() == "UserDataNode");
    REQUIRE(ctor.get("prototype").get("isUserDataNode").isUndefined());

    auto data = JSValue::object();
    data.set("rotation", 1.5);
    const auto node = nativeUserDataNodeBinding("userData").call({}, {"rotation", "float", data});
    REQUIRE(node.get("isUserDataNode").isUndefined());
    REQUIRE(node.get("userData").strictEquals(data));
    REQUIRE(node.get("property").toString() == "rotation");
    REQUIRE(node.get("uniformType").toString() == "float");
}

TEST_CASE("UserDataNode updateReference prefers explicit data and falls back to state object data") {
    auto data = JSValue::object();
    data.set("rotation", 1.5);
    auto fallback = JSValue::object();
    fallback.set("rotation", 2.5);
    auto object = JSValue::object();
    object.set("userData", fallback);
    auto state = JSValue::object();
    state.set("object", object);

    const auto explicitNode = UserDataNode("rotation", "float", data).object();
    REQUIRE(explicitNode.get("updateReference").call(explicitNode, {state}).strictEquals(data));
    REQUIRE(explicitNode.get("reference").strictEquals(data));

    const auto fallbackNode = UserDataNode("rotation", "float").object();
    REQUIRE(fallbackNode.get("updateReference").call(fallbackNode, {state}).strictEquals(fallback));
    REQUIRE(fallbackNode.get("reference").strictEquals(fallback));
}

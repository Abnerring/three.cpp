#include "nodes/core/Node.h"
#include "nodes/core/NodeFrame.h"
#include "catch.hpp"

TEST_CASE("Node fields and aliases share the live managed object") {
    Node first, second("vec3");
    REQUIRE(second.id.toNumber() == first.id.toNumber() + 1);
    REQUIRE(first.nodeType.isNull()); REQUIRE(second.nodeType.strictEquals("vec3"));
    REQUIRE(first.object().getPrototype().sameValue(Node::prototype()));
    first.version = "4"; first.needsUpdate = 1; REQUIRE(first.version.strictEquals("4"));
    first.needsUpdate = true; REQUIRE(first.version.strictEquals(5));
    Node alias = first; alias.name = "shared"; REQUIRE(first.name.strictEquals("shared"));
    auto descriptor = first.object().getOwnPropertyDescriptor("id");
    REQUIRE(descriptor->writable.value()); REQUIRE(descriptor->enumerable.value());
}
TEST_CASE("Node callbacks are bound and run through the real NodeFrame") {
    Node node; NodeFrame frame; frame.frameId = 1;
    auto log = JSValue::array();
    auto callback = JSValue::function([](const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>& captures) -> JSValue {
        auto log = captures[0]; log.set(std::to_string(log.length()), self);
        return args[0].get("frameId");
    }, {log}, "step", 1);
    auto object = node.object();
    REQUIRE(object.get("onFrameUpdate").call(object, {callback}).sameValue(object));
    frame.updateNode(object); frame.updateNode(object);
    REQUIRE(log.length() == 1); REQUIRE(log.get("0").sameValue(object));
    REQUIRE(object.get("update").call(nullptr, {frame.object()}).strictEquals(1));
    REQUIRE(log.length() == 2);
    REQUIRE(object.get("update").get("name").strictEquals("bound step"));
}
TEST_CASE("Node cache observes children only after invalidation") {
    Node root, child;
    auto object = root.object(); object.set("childNode", child.object());
    const auto before = root.getCacheKey();
    child.id = 999;
    REQUIRE(root.getCacheKey().sameValue(before));
    root.needsUpdate = true;
    REQUIRE_FALSE(root.getCacheKey().sameValue(before));
    object.set("selfNode", object);
    REQUIRE(root.getCacheKey(true).type() == JSValue::Type::Number);
    auto iterator = object.get("getChildren").call(object);
    REQUIRE(iterator.get("next").call(iterator).get("value").sameValue(child.object()));
}
TEST_CASE("Node JSON de-duplicates shared children and restores their identities") {
    Node root, child;
    auto object = root.object(), childObject = child.object();
    object.set("_uuid", "root"); childObject.set("_uuid", "child");
    object.set("firstNode", childObject); object.set("secondNode", childObject);
    const auto json = root.toJSON();
    REQUIRE(json.get("nodes").length() == 1);
    REQUIRE(json.get("inputNodes").get("firstNode").strictEquals("child"));
    REQUIRE(json.get("inputNodes").get("secondNode").strictEquals("child"));
    REQUIRE_FALSE(json.hasOwn("meta"));
    REQUIRE_FALSE(json.get("nodes").get("0").hasOwn("metadata"));
    auto input = JSValue::object(), meta = JSValue::object(), nodes = JSValue::object();
    nodes.set("child", childObject); meta.set("nodes", nodes); input.set("meta", meta); input.set("inputNodes", json.get("inputNodes"));
    Node restored; restored.object().get("deserialize").call(restored.object(), {input});
    REQUIRE(restored.object().get("firstNode").sameValue(childObject));
    REQUIRE(restored.object().get("secondNode").sameValue(childObject));
}

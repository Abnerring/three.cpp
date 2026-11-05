#include "nodes/utils/ReflectorNode.h"

#include "core/JSValue.h"

#include "catch.hpp"

TEST_CASE("ReflectorBaseNode applies the pinned options") {
    auto textureNode = JSValue::object(nullptr);
    const auto base = nativeReflectorBaseNodeConstructor().construct({textureNode});
    REQUIRE(base.get("textureNode").strictEquals(textureNode));
    REQUIRE(base.get("resolutionScale").toNumber() == Approx(1));
    REQUIRE_FALSE(base.get("generateMipmaps").toBoolean());
    REQUIRE(base.get("bounces").toBoolean());
    REQUIRE_FALSE(base.get("depth").toBoolean());
    REQUIRE(base.get("samples").toNumber() == Approx(0));
    REQUIRE(base.get("target").toBoolean());
    REQUIRE(base.get("virtualCameras").toBoolean());
    REQUIRE(base.get("renderTargets").toBoolean());
    REQUIRE_FALSE(base.get("isCacheable").call(base, {}).toBoolean());
}
TEST_CASE("ReflectorNode constructs its base node and depth access") {
    const auto node = nativeReflectorNodeConstructor().construct({});
    REQUIRE(node.get("type").toString() == "ReflectorNode");
    REQUIRE(node.get("_reflectorBaseNode").toBoolean());
    REQUIRE(node.get("_depthNode").isNull());
    // The depth node is only available when the reflector was created with depth enabled.
    REQUIRE_THROWS(node.get("getDepthNode").call(node, {}));
}

TEST_CASE("ReflectorNode creates and caches the depth node when depth is enabled") {
    auto parameters = JSValue::object(nullptr);
    parameters.set("depth", true);
    parameters.set("resolutionScale", JSValue(0.5));
    const auto node = nativeReflectorNodeConstructor().construct({parameters});
    const auto baseNode = node.get("_reflectorBaseNode");
    REQUIRE(baseNode.get("depth").toBoolean());
    REQUIRE(baseNode.get("resolutionScale").toNumber() == Approx(0.5));

    const auto depthNode = node.get("getDepthNode").call(node, {});
    REQUIRE(depthNode.get("type").toString() == "ReflectorNode");
    REQUIRE(depthNode.get("_reflectorBaseNode").strictEquals(baseNode));
    REQUIRE(node.get("getDepthNode").call(node, {}).strictEquals(depthNode));

    // The clone keeps the same base node and copies the sampling fields.
    const auto cloned = node.get("clone").call(node, {});
    REQUIRE(cloned.get("_reflectorBaseNode").strictEquals(baseNode));
    REQUIRE(cloned.get("uvNode").strictEquals(node.get("uvNode")));
}

TEST_CASE("ReflectorNode dispose releases the base node") {
    auto node = nativeReflectorNodeConstructor().construct({});
    auto baseNode = JSValue::object(nullptr);
    baseNode.set("_disposes", JSValue(0));
    baseNode.set("dispose", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            self.set("_disposes", JSValue(self.get("_disposes").toNumber() + 1));
            return JSValue{};
        }));
    node.set("_reflectorBaseNode", baseNode);
    REQUIRE_NOTHROW(node.get("dispose").call(node, {}));
    REQUIRE(baseNode.get("_disposes").toNumber() == Approx(1));
}
TEST_CASE("ReflectorNode factory and resolution alias") {
    auto base = nativeReflectorBaseNodeConstructor().construct({JSValue::object(nullptr)});
    base.set("resolutionScale", JSValue(2));
    REQUIRE(base.get("resolution").toNumber() == Approx(2));
    base.set("resolution", JSValue(3));
    REQUIRE(base.get("resolutionScale").toNumber() == Approx(3));

    const auto node = nativeReflectorNodeBinding("reflector").call({}, {});
    REQUIRE(node.get("type").toString() == "ReflectorNode");
    REQUIRE(node.get("_reflectorBaseNode").toBoolean());
}
#include "nodes/display/ToonOutlinePassNode.h"

#include "core/JSValue.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("ToonOutlinePassNode creates the outline material") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto color = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)});
    auto thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.003)});
    auto alpha = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    const auto node = nativeToonOutlinePassNodeConstructor().construct(
        {scene, camera, color, thickness, alpha});
    REQUIRE(node.get("isPassNode").toBoolean());
    REQUIRE(node.get("name").toString() == "Outline Pass");
    REQUIRE(node.get("colorNode").strictEquals(color));
    REQUIRE(node.get("thicknessNode").strictEquals(thickness));
    REQUIRE(node.get("alphaNode").strictEquals(alpha));
    const auto material = node.get("_outlineMaterial");
    REQUIRE(material.get("isMeshToonOutlineMaterial").toBoolean());
    REQUIRE(material.get("name").toString() == "Toon_Outline");
    // BackSide is 1 in the pinned constants table.
    REQUIRE(material.get("side").toNumber() == Approx(1));
    REQUIRE(material.get("colorNode").toBoolean());
    REQUIRE(material.get("vertexNode").toBoolean());
}

TEST_CASE("ToonOutlinePassNode reuses and caches the outline materials") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto color = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)});
    auto thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.003)});
    auto alpha = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    const auto node = nativeToonOutlinePassNodeConstructor().construct(
        {scene, camera, color, thickness, alpha});

    // Plain materials without a position node share the default outline material.
    auto plain = JSValue::object(nullptr);
    const auto shared = node.get("_getOutlineMaterial").call(node, {plain});
    REQUIRE(shared.strictEquals(node.get("_outlineMaterial")));

    // A material with a position node gets its own cached outline material.
    auto custom = JSValue::object(nullptr);
    custom.set("positionNode", JSValue::object(nullptr));
    custom.set("version", JSValue(1));
    for (const char* method : {"addEventListener", "removeEventListener"}) {
        custom.set(method, JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                return JSValue{};
            }));
    }
    const auto outline = node.get("_getOutlineMaterial").call(node, {custom});
    REQUIRE_FALSE(outline.strictEquals(node.get("_outlineMaterial")));
    REQUIRE(node.get("_getOutlineMaterial").call(node, {custom}).strictEquals(outline));
    REQUIRE(outline.get("positionNode").strictEquals(custom.get("positionNode")));
    REQUIRE(outline.get("version").toNumber() == Approx(1));
}

TEST_CASE("ToonOutlinePassNode factory builds the node with defaults") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    const auto node = nativeToonOutlinePassNodeBinding("toonOutlinePass").call({}, {scene, camera});
    REQUIRE(node.get("isPassNode").toBoolean());
    REQUIRE(node.get("colorNode").toBoolean());
    REQUIRE(node.get("thicknessNode").toBoolean());
    REQUIRE(node.get("alphaNode").toBoolean());
}
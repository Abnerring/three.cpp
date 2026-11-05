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
TEST_CASE("ToonOutlinePassNode updateBefore wraps and restores the render object function") {
    auto scene = JSValue::object(nullptr);
    auto cameraLayers = JSValue::object(nullptr);
    cameraLayers.set("mask", JSValue(1));
    auto camera = JSValue::object(nullptr);
    camera.set("near", JSValue(0.1));
    camera.set("far", JSValue(50));
    camera.set("layers", cameraLayers);
    auto sentinel = JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        });
    auto renderer = JSValue::object(nullptr);
    renderer.set("_restored", JSValue(false));
    renderer.set("autoClear", true);
    renderer.set("autoClearColor", true);
    renderer.set("autoClearDepth", true);
    renderer.set("autoClearStencil", true);
    renderer.set("transparent", true);
    renderer.set("opaque", true);
    renderer.set("lighting", JSValue(nullptr));
    renderer.set("contextNode", JSValue(nullptr));
    renderer.set("getRenderObjectFunction", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue::function(
                +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                    return JSValue{};
                });
        }));
    // The setter records whether the sentinel was restored at the end.
    renderer.set("setRenderObjectFunction", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>& args,
            const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            const auto value = args.size() > 0 ? args[0] : JSValue{};
            const auto calls = self.get("_setCalls").toNumber() + 1;
            self.set("_setCalls", JSValue(calls));
            if (calls == 1) self.set("_wrapper", value);
            if (calls == 2) self.set("_restored", JSValue(value.isCallable()));
            return JSValue{};
        }));
    renderer.set("_setCalls", JSValue(0));
    renderer.set("getOutputRenderTarget", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue(nullptr);
        }));
    renderer.set("getDrawingBufferSize", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            auto size = args.size() > 0 ? args[0] : JSValue::object(nullptr);
            size.set("width", JSValue(64));
            size.set("height", JSValue(64));
            return size;
        }));
    renderer.set("getRenderTarget", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue(nullptr);
        }));
    renderer.set("getMRT", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue(nullptr);
        }));
    renderer.set("setRenderTarget", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    renderer.set("setMRT", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    renderer.set("render", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    renderer.set("_renderObjectCount", JSValue(0));
    renderer.set("renderObject", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>& args,
            const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            const auto count = static_cast<double>(self.get("_renderObjectCount").toNumber());
            self.set("_renderObjectCount", JSValue(count + 1));
            const auto material = args.size() > 4 ? args[4] : JSValue{};
            if (count == 0) {
                self.set("_firstIsOutline",
                         JSValue(material.get("isMeshToonOutlineMaterial").toBoolean()));
            }
            return JSValue{};
        }));
    auto color = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)});
    auto thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.003)});
    auto alpha = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    const auto node = nativeToonOutlinePassNodeConstructor().construct(
        {scene, camera, color, thickness, alpha});
    auto frame = JSValue::object(nullptr);
    frame.set("renderer", renderer);
    REQUIRE_NOTHROW(node.get("updateBefore").call(node, {frame}));
    REQUIRE(renderer.get("_setCalls").toNumber() == Approx(2));
    REQUIRE(renderer.get("_restored").toBoolean());

    // The installed wrapper draws the outline material first for a toon material without wireframe.
    const auto wrapper = renderer.get("_wrapper");
    REQUIRE(wrapper.isCallable());
    auto toon = JSValue::object(nullptr);
    toon.set("isMeshToonNodeMaterial", true);
    toon.set("wireframe", false);
    wrapper.call({}, {JSValue::object(nullptr), scene, camera, JSValue::object(nullptr), toon,
                      JSValue(0), JSValue::object(nullptr), JSValue(nullptr)});
    REQUIRE(renderer.get("_renderObjectCount").toNumber() == Approx(2));
    REQUIRE(renderer.get("_firstIsOutline").toBoolean());
}

TEST_CASE("ToonOutlinePassNode dispose clears the material cache") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto color = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)});
    auto thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.003)});
    auto alpha = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    auto node = nativeToonOutlinePassNodeConstructor().construct(
        {scene, camera, color, thickness, alpha});
    auto custom = JSValue::object(nullptr);
    custom.set("positionNode", JSValue::object(nullptr));
    custom.set("version", JSValue(1));
    custom.set("_removes", JSValue(0));
    custom.set("addEventListener", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    custom.set("removeEventListener", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            self.set("_removes", JSValue(self.get("_removes").toNumber() + 1));
            return JSValue{};
        }));
    node.get("_getOutlineMaterial").call(node, {custom});
    const auto cache = node.get("_materialCache");
    REQUIRE(cache.get("size").toNumber() == Approx(1));
    auto target = JSValue::object(nullptr);
    target.set("dispose", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    node.set("renderTarget", target);
    REQUIRE_NOTHROW(node.get("dispose").call(node, {}));
    REQUIRE(cache.get("size").toNumber() == Approx(0));
    REQUIRE(custom.get("_removes").toNumber() == Approx(1));
}
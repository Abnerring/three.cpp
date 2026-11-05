#include "nodes/utils/RTTNode.h"

#include "core/RenderTarget.h"
#include "nodes/accessors/TextureNode.h"
#include "constants.h"
#include "nodes/core/NodeConstants.h"

#include "catch.hpp"

TEST_CASE("RTTNode wraps a node and owns a render target") {
    const auto module = nativeRTTNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("RTTNode"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("RTTNode"));

    auto source = JSValue::object(nullptr);
    source.set("isNode", true);
    const auto node = ctor.construct({source});
    REQUIRE(node.get("type").strictEquals("RTTNode"));
    REQUIRE(node.get("isRTTNode").strictEquals(true));
    REQUIRE(node.get("node").sameValue(source));
    REQUIRE(node.get("width").isNull());
    REQUIRE(node.get("height").isNull());
    REQUIRE(node.get("renderTarget").get("texture").sameValue(node.get("value")));
    REQUIRE(node.get("textureNeedsUpdate").strictEquals(true));
    REQUIRE(node.get("autoUpdate").strictEquals(true));
    REQUIRE(node.get("_resolutionScale").toNumber() == Approx(1));
    // The quad mesh owns a fresh node material.
    REQUIRE(node.get("_quadMesh").get("material").get("isNodeMaterial").strictEquals(true));
    // updateBeforeType is the frame update constant, whose value is the string "frame".
    REQUIRE(node.get("updateBeforeType").strictEquals("frame"));

    // Width drives the auto resize flag: no width means it resizes itself.
    REQUIRE(node.get("autoResize").strictEquals(true));
    const auto sized = ctor.construct({source, JSValue(4), JSValue(4)});
    REQUIRE(sized.get("autoResize").strictEquals(false));
    REQUIRE(sized.get("width").toNumber() == Approx(4));
    REQUIRE(sized.get("renderTarget").get("width").toNumber() == Approx(4));
}

TEST_CASE("RTTNode size and resolution scale follow the source arithmetic") {
    auto source = JSValue::object(nullptr);
    source.set("isNode", true);
    const auto ctor = nativeRTTNodeConstructor();

    // Options carry a resolution scale; size is floored after scaling.
    auto options = JSValue::object(nullptr);
    options.set("resolutionScale", 0.5);
    const auto scaled = ctor.construct({source, JSValue(10), JSValue(5), options});
    scaled.get("setSize").call(scaled, {JSValue(10), JSValue(5)});
    REQUIRE(scaled.get("renderTarget").get("width").toNumber() == Approx(5));
    REQUIRE(scaled.get("renderTarget").get("height").toNumber() == Approx(2));
    REQUIRE(scaled.get("textureNeedsUpdate").strictEquals(true));

    // A self resizing node keeps its size until the renderer drives it; the scale is still stored.
    const auto autoNode = ctor.construct({source});
    const auto returned = autoNode.get("setResolutionScale").call(autoNode, {JSValue(0.25)});
    REQUIRE(returned.sameValue(autoNode));
    REQUIRE(autoNode.get("getResolutionScale").call(autoNode, {}).toNumber() == Approx(0.25));

    // An explicitly sized node resizes when the scale changes.
    const auto sized = ctor.construct({source, JSValue(8), JSValue(8)});
    sized.get("setResolutionScale").call(sized, {JSValue(0.5)});
    REQUIRE(sized.get("renderTarget").get("width").toNumber() == Approx(4));
    REQUIRE(sized.get("renderTarget").get("height").toNumber() == Approx(4));
}

TEST_CASE("RTTNode clone copies the texture wiring and dispose runs") {
    auto source = JSValue::object(nullptr);
    source.set("isNode", true);
    auto options = JSValue::object(nullptr);
    options.set("autoUpdate", false);
    const auto node = nativeRTTNodeConstructor().construct({source, JSValue(2), JSValue(2), options});
    REQUIRE(node.get("autoUpdate").strictEquals(false));

    const auto copy = node.get("clone").call(node, {});
    REQUIRE(copy.get("isTextureNode").strictEquals(true));
    REQUIRE_FALSE(copy.sameValue(node));
    // The clone points back at this node and keeps the sampler wiring.
    REQUIRE(copy.get("referenceNode").sameValue(node));
    REQUIRE(copy.get("sampler").sameValue(node.get("sampler")));

    REQUIRE_NOTHROW(node.get("dispose").call(node, {}));
}

TEST_CASE("RTTNode module helpers convert nodes to textures") {
    const auto module = nativeRTTNodeModule();
    REQUIRE(module.get("rtt").get("length").strictEquals(1));
    REQUIRE(module.get("convertToTexture").get("length").strictEquals(1));

    // Sample and texture nodes pass through unchanged.
    auto sample = JSValue::object(nullptr);
    sample.set("isSampleNode", true);
    REQUIRE(module.get("convertToTexture").call({}, {sample}).sameValue(sample));

    auto texture = JSValue::object(nullptr);
    texture.set("isTextureNode", true);
    REQUIRE(module.get("convertToTexture").call({}, {texture}).sameValue(texture));

    // Pass nodes answer their own texture node.
    auto produced = JSValue::object(nullptr);
    produced.set("isTextureNode", true);
    auto pass = JSValue::object(nullptr);
    pass.set("isPassNode", true);
    pass.set("getTextureNode", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            return captures[0];
        },
        {produced}, "getTextureNode", 0));
    REQUIRE(module.get("convertToTexture").call({}, {pass}).sameValue(produced));

    // Everything else is wrapped into an RTT node.
    auto plain = JSValue::object(nullptr);
    plain.set("isNode", true);
    const auto wrapped = module.get("convertToTexture").call({}, {plain});
    REQUIRE(wrapped.get("isRTTNode").strictEquals(true));
    REQUIRE(wrapped.get("node").sameValue(plain));
}

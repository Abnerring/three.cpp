#include "nodes/display/PassNode.h"

#include "core/JSValue.h"

#include "catch.hpp"

TEST_CASE("PassTextureNode extends the texture node") {
    auto passNode = JSValue::object(nullptr);
    auto texture = JSValue::object(nullptr);
    texture.set("isTexture", true);
    const auto node = nativePassTextureNodeConstructor().construct({passNode, texture});
    REQUIRE(node.get("type").toString() == "PassTextureNode");
    REQUIRE(node.get("isPassTextureNode").toBoolean());
    REQUIRE(node.get("passNode").strictEquals(passNode));
}

TEST_CASE("PassMultipleTextureNode stores its texture name") {
    auto passNode = JSValue::object(nullptr);
    const auto node = nativePassMultipleTextureNodeConstructor().construct(
        {passNode, JSValue("depth")});
    REQUIRE(node.get("type").toString() == "PassMultipleTextureNode");
    REQUIRE(node.get("isPassMultipleTextureNode").toBoolean());
    REQUIRE(node.get("textureName").toString() == "depth");
    REQUIRE_FALSE(node.get("previousTexture").toBoolean());
}

TEST_CASE("PassMultipleTextureNode updateTexture reads the pass texture") {
    auto sampled = JSValue::object(nullptr);
    sampled.set("isTexture", true);
    auto passNode = JSValue::object(nullptr);
    passNode.set("getTexture", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            auto texture = JSValue::object(nullptr);
            texture.set("isTexture", true);
            return texture;
        }));
    auto node = nativePassMultipleTextureNodeConstructor().construct({passNode, JSValue("output")});
    node.get("updateTexture").call(node, {});
    REQUIRE(node.get("value").get("isTexture").toBoolean());
}
TEST_CASE("PassNode constructor initialises the pass state") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto options = JSValue::object(nullptr);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera, options});
    REQUIRE(node.get("isPassNode").toBoolean());
    REQUIRE(node.get("global").toBoolean());
    REQUIRE(node.get("nodeType").toString() == "vec4");
    REQUIRE(node.get("scope").toString() == "color");
    REQUIRE(node.get("scene").strictEquals(scene));
    REQUIRE(node.get("camera").strictEquals(camera));
    REQUIRE(node.get("_width").toNumber() == Approx(1));
    REQUIRE(node.get("_height").toNumber() == Approx(1));
    REQUIRE(node.get("_resolutionScale").toNumber() == Approx(1));
    REQUIRE(node.get("renderTarget").toBoolean());
    REQUIRE(node.get("renderTarget").get("texture").get("name").toString() == "output");
    // A colour pass with the default depth buffer option creates the depth texture too.
    REQUIRE(node.get("_textures").get("output").toBoolean());
    REQUIRE(node.get("_textures").get("depth").toBoolean());
    REQUIRE(node.get("_textures").get("depth").get("name").toString() == "depth");
    REQUIRE(node.get("autoClear").toBoolean());
    REQUIRE(node.get("overrideMaterial").isNull());
    REQUIRE(node.get("lighting").isNull());
    REQUIRE(node.get("_mrt").isNull());
    REQUIRE(node.get("_viewport").isNull());
    REQUIRE(node.get("_scissor").isNull());
}

TEST_CASE("PassNode scope statics and factories answer the pass node") {
    const auto colorScope = nativePassNodeBinding("PassNode").get("COLOR");
    REQUIRE(colorScope.toString() == "color");
    const auto depthScope = nativePassNodeBinding("PassNode").get("DEPTH");
    REQUIRE(depthScope.toString() == "depth");

    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    const auto pass = nativePassNodeBinding("pass").call({}, {scene, camera});
    REQUIRE(pass.get("isPassNode").toBoolean());
    REQUIRE(pass.get("scope").toString() == "color");
    const auto depthPass = nativePassNodeBinding("depthPass").call({}, {scene, camera});
    REQUIRE(depthPass.get("scope").toString() == "depth");
    // The depth scope forces the depth texture even when the option asks to skip the depth buffer.
    auto depthOptions = JSValue::object(nullptr);
    depthOptions.set("depthBuffer", false);
    const auto forcedDepth = nativePassNodeBinding("depthPass").call({}, {scene, camera, depthOptions});
    REQUIRE(forcedDepth.get("_textures").get("depth").toBoolean());
}

TEST_CASE("PassNode options can disable the depth buffer and the auto clear flags") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto options = JSValue::object(nullptr);
    options.set("depthBuffer", false);
    options.set("autoClear", false);
    options.set("autoClearColor", false);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera, options});
    REQUIRE_FALSE(node.get("_textures").get("depth").toBoolean());
    REQUIRE_FALSE(node.get("autoClear").toBoolean());
    REQUIRE_FALSE(node.get("autoClearColor").toBoolean());
    REQUIRE(node.get("autoClearDepth").toBoolean());
}

TEST_CASE("PassNode module exposes the passTexture factory") {
    auto passNode = JSValue::object(nullptr);
    auto texture = JSValue::object(nullptr);
    texture.set("isTexture", true);
    const auto node = nativePassNodeBinding("passTexture").call({}, {passNode, texture});
    REQUIRE(node.get("type").toString() == "PassTextureNode");
    REQUIRE(node.get("passNode").strictEquals(passNode));
}
TEST_CASE("PassNode texture accessors clone and toggle the pass textures") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto options = JSValue::object(nullptr);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera, options});
    REQUIRE_FALSE(node.get("isCacheable").call(node, {}).toBoolean());

    // The output texture is the render target texture itself.
    const auto output = node.get("getTexture").call(node, {JSValue("output")});
    REQUIRE(output.strictEquals(node.get("renderTarget").get("texture")));
    // The depth texture exists for a default colour pass.
    const auto depthTexture = node.get("getTexture").call(node, {JSValue("depth")});
    REQUIRE(depthTexture.get("name").toString() == "depth");

    // Any other name clones the render target texture and records it.
    const auto custom = node.get("getTexture").call(node, {JSValue("custom")});
    REQUIRE(custom.get("name").toString() == "custom");
    REQUIRE(node.get("_textures").get("custom").strictEquals(custom));
    REQUIRE(node.get("getTexture").call(node, {JSValue("custom")}).strictEquals(custom));

    // Previous textures are cloned on demand and cached.
    const auto previous = node.get("getPreviousTexture").call(node, {JSValue("output")});
    REQUIRE(previous.toBoolean());
    REQUIRE(node.get("getPreviousTexture").call(node, {JSValue("output")}).strictEquals(previous));

    // Toggling swaps the current and previous textures for that name.
    node.get("toggleTexture").call(node, {JSValue("output")});
    REQUIRE(node.get("_textures").get("output").strictEquals(previous));
    REQUIRE(node.get("_previousTextures").get("output").strictEquals(output));
}

TEST_CASE("PassNode depth texture access raises when the pass has no depth") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto options = JSValue::object(nullptr);
    options.set("depthBuffer", false);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera, options});
    REQUIRE_THROWS(node.get("getTexture").call(node, {JSValue("depth")}));
}

TEST_CASE("PassNode resolution layers and mrt setters round trip") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera});
    REQUIRE(node.get("setResolutionScale").call(node, {JSValue(2)}).strictEquals(node));
    REQUIRE(node.get("getResolutionScale").call(node, {}).toNumber() == Approx(2));
    auto layers = JSValue::object(nullptr);
    node.get("setLayers").call(node, {layers});
    REQUIRE(node.get("getLayers").call(node, {}).strictEquals(layers));
    auto mrt = JSValue::object(nullptr);
    node.get("setMRT").call(node, {mrt});
    REQUIRE(node.get("getMRT").call(node, {}).strictEquals(mrt));
}
TEST_CASE("PassNode node accessors cache the texture and depth nodes") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera});

    const auto textureNode = node.get("getTextureNode").call(node, {});
    REQUIRE(textureNode.get("type").toString() == "PassMultipleTextureNode");
    REQUIRE(node.get("getTextureNode").call(node, {JSValue("output")}).strictEquals(textureNode));

    const auto previousNode = node.get("getPreviousTextureNode").call(node, {});
    REQUIRE(previousNode.toBoolean());
    REQUIRE_FALSE(previousNode.strictEquals(textureNode));
    REQUIRE(previousNode.get("previousTexture").toBoolean());

    const auto viewZ = node.get("getViewZNode").call(node, {});
    REQUIRE(viewZ.toBoolean());
    REQUIRE(viewZ.get("isNode").toBoolean());
    REQUIRE(node.get("getViewZNode").call(node, {JSValue("depth")}).strictEquals(viewZ));

    const auto linearDepth = node.get("getLinearDepthNode").call(node, {});
    REQUIRE(linearDepth.toBoolean());
    REQUIRE(linearDepth.get("isNode").toBoolean());
}
TEST_CASE("PassNode setSize scales the target and the scissor and viewport") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera});
    node.get("setResolutionScale").call(node, {JSValue(2)});
    node.get("setScissor").call(node, {JSValue(1), JSValue(2), JSValue(3), JSValue(4)});
    node.get("setViewport").call(node, {JSValue(5), JSValue(6), JSValue(7), JSValue(8)});
    node.get("setSize").call(node, {JSValue(100), JSValue(50)});

    const auto renderTarget = node.get("renderTarget");
    REQUIRE(renderTarget.get("width").toNumber() == Approx(200));
    REQUIRE(renderTarget.get("height").toNumber() == Approx(100));
    REQUIRE(renderTarget.get("scissorTest").toBoolean());
    const auto scissor = renderTarget.get("scissor");
    REQUIRE(scissor.get("x").toNumber() == Approx(2));
    REQUIRE(scissor.get("y").toNumber() == Approx(4));
    REQUIRE(scissor.get("z").toNumber() == Approx(6));
    REQUIRE(scissor.get("w").toNumber() == Approx(8));
    const auto viewport = renderTarget.get("viewport");
    REQUIRE(viewport.get("x").toNumber() == Approx(10));
    REQUIRE(viewport.get("y").toNumber() == Approx(12));
    REQUIRE(viewport.get("z").toNumber() == Approx(14));
    REQUIRE(viewport.get("w").toNumber() == Approx(16));

    // Clearing the scissor disables the test on the next size update.
    node.get("setScissor").call(node, {JSValue(nullptr)});
    REQUIRE(node.get("_scissor").isNull());
    node.get("setSize").call(node, {JSValue(10), JSValue(10)});
    REQUIRE_FALSE(renderTarget.get("scissorTest").toBoolean());
}

TEST_CASE("PassNode setup takes the renderer buffer type and depth type") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto options = JSValue::object(nullptr);
    options.set("samples", JSValue(2));
    const auto node = nativePassNodeConstructor().construct(
        {JSValue("color"), scene, camera, options});
    auto renderer = JSValue::object(nullptr);
    renderer.set("samples", JSValue(8));
    renderer.set("reversedDepthBuffer", true);
    renderer.set("getOutputBufferType", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue(1009); // HalfFloatType
        }));
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    const auto result = node.get("setup").call(node, {builder});
    REQUIRE(result.get("type").toString() == "PassMultipleTextureNode");
    const auto renderTarget = node.get("renderTarget");
    // The explicit option wins over the renderer sample count.
    REQUIRE(renderTarget.get("samples").toNumber() == Approx(2));
    REQUIRE(renderTarget.get("texture").get("type").toNumber() == Approx(1009));
    REQUIRE(renderTarget.get("depthTexture").get("type").toNumber() == Approx(1015));
}

TEST_CASE("PassNode dispose disposes the render target") {
    auto scene = JSValue::object(nullptr);
    auto camera = JSValue::object(nullptr);
    auto node = nativePassNodeConstructor().construct({JSValue("color"), scene, camera});
    auto target = JSValue::object(nullptr);
    target.set("_disposes", JSValue(0));
    target.set("dispose", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            self.set("_disposes", JSValue(self.get("_disposes").toNumber() + 1));
            return JSValue{};
        }));
    node.set("renderTarget", target);
    REQUIRE_NOTHROW(node.get("dispose").call(node, {}));
    REQUIRE(target.get("_disposes").toNumber() == Approx(1));
}
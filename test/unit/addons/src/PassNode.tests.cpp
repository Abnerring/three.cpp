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
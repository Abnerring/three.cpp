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
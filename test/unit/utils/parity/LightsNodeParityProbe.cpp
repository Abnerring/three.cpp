#include "nodes/lighting/LightsNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"

#include <iostream>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue makeBuilder(const JSValue& data) {
    auto context = JSValue::object(nullptr);
    context.set("materialLightings", JSValue::array());
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);
    builder.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            return captures[0];
        },
        {data}, "getDataFromNode", 1));
    return builder;
}
JSValue nodeLight(double id, const char* hash) {
    auto light = JSValue::object(nullptr);
    light.set("isNode", true);
    light.set("id", id);
    light.set("getHash", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            return captures[0];
        },
        {JSValue(hash)}, "getHash", 0));
    return light;
}
}  // namespace

int main() {
    try {
        const auto module = nativeLightsNodeModule();
        const auto ctor = nativeLightsNodeConstructor();
        const auto factory = nativeLightsFactory();

        const auto node = ctor.construct();
        const auto other = ctor.construct();

        auto list = JSValue::array();
        list.set(JSValue(0), JSValue::object(nullptr));
        const auto returned = node.get("setLights").call(node, {list});

        // The cache key case: one plain light and one spot light with no map and no colour node.
        const auto keyNode = ctor.construct();
        auto plain = JSValue::object(nullptr);
        plain.set("id", 7);
        plain.set("castShadow", false);
        auto spot = JSValue::object(nullptr);
        spot.set("id", 3);
        spot.set("castShadow", true);
        spot.set("isSpotLight", true);
        spot.set("map", nullptr);
        spot.set("colorNode", nullptr);
        auto keyList = JSValue::array();
        keyList.set(JSValue(0), plain);
        keyList.set(JSValue(1), spot);
        keyNode.get("setLights").call(keyNode, {keyList});
        const auto cacheKey = keyNode.get("customCacheKey").call(keyNode, {});

        // The hash case: two node lights given out of order.
        const auto hashNode = ctor.construct();
        auto hashList = JSValue::array();
        hashList.set(JSValue(0), nodeLight(2, "b"));
        hashList.set(JSValue(1), nodeLight(1, "a"));
        hashNode.get("setLights").call(hashNode, {hashList});
        auto data = JSValue::object(nullptr);
        const auto builder = makeBuilder(data);
        const auto hash = hashNode.get("getHash").call(hashNode, {builder});
        const auto lightNodes = hashNode.get("getLightNodes").call(hashNode, {builder});

        const auto built = factory.call({}, {list});
        const auto empty = factory.call({}, {});

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("instanceType", node.get("type"));
        output.set("global", node.get("global"));
        output.set("emptyLightsLength", node.get("_lights").length());
        output.set("sharedTotalDiffuse", node.get("totalDiffuseNode").sameValue(other.get("totalDiffuseNode")));
        output.set("sharedTotalSpecular", node.get("totalSpecularNode").sameValue(other.get("totalSpecularNode")));
        output.set("sharedOutgoingLight", node.get("outgoingLightNode").sameValue(other.get("outgoingLightNode")));
        output.set("isCacheable", node.get("isCacheable").call(node, {}));
        output.set("setLightsReturnsThis", returned.sameValue(node));
        output.set("getLightsIsSameArray", node.get("getLights").call(node, {}).sameValue(list));
        output.set("builtinIsSameArray", node.get("getBuiltinLights").call(node, {}).sameValue(list));
        output.set("hasLights", node.get("hasLights"));
        output.set("cacheKey", cacheKey);
        output.set("hash", hash);
        output.set("hashCached", data.get("lightNodesHash"));
        output.set("lightNodeCount", lightNodes.length());
        output.set("firstLightNodeId", lightNodes.get(JSValue(0)).get("id"));
        output.set("factoryName", factory.get("name"));
        output.set("factoryLength", factory.get("length"));
        output.set("factoryLightsIsSameArray", built.get("getLights").call(built, {}).sameValue(list));
        output.set("factoryEmptyLength", empty.get("getLights").call(empty, {}).length());
        output.set("setupLength", node.get("setup").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}

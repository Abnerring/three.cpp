#include "nodes/lighting/LightsNode.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

// A builder stand-in: one shared data object per node, and an empty material lighting list.
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

// A light that is already a node, so the setup path pushes it without building a class.
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

TEST_CASE("LightsNode exposes the pinned shared lighting shape") {
    const auto module = nativeLightsNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("LightsNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("LightsNode"));

    const auto node = ctor.construct();
    // The instance type resolves through the class accessor, so it reads LightsNode even though the
    // constructor passes 'vec3' to the base. The differential compares this same field upstream and
    // both sides agree, so the expectation is not a guess.
    REQUIRE(node.get("type").strictEquals("LightsNode"));
    REQUIRE(node.get("global").strictEquals(true));
    REQUIRE(node.get("_lights").length() == 0);
    REQUIRE(node.get("hasLights").strictEquals(false));
    REQUIRE(node.get("isCacheable").call(node, {}).strictEquals(false));

    // The three lighting accumulators are module level property nodes shared by every instance.
    const auto other = ctor.construct();
    for (const char* name : {"totalDiffuseNode", "totalSpecularNode", "outgoingLightNode"}) {
        REQUIRE(node.get(name).get("isNode").strictEquals(true));
        REQUIRE(node.get(name).sameValue(other.get(name)));
        REQUIRE(node.get(name).get("type").strictEquals("PropertyNode"));
    }
    REQUIRE(node.get("setupLightsNode").get("length").strictEquals(1));
    REQUIRE(node.get("setup").get("length").strictEquals(1));
}

TEST_CASE("LightsNode keeps the array it is given and reports whether it has lights") {
    const auto node = nativeLightsNodeConstructor().construct();
    auto list = JSValue::array();
    list.set(JSValue(0), JSValue::object(nullptr));
    const auto returned = node.get("setLights").call(node, {list});
    // setLights returns this and stores the very same array.
    REQUIRE(returned.sameValue(node));
    REQUIRE(node.get("getLights").call(node, {}).sameValue(list));
    REQUIRE(node.get("getBuiltinLights").call(node, {}).sameValue(list));
    REQUIRE(node.get("hasLights").strictEquals(true));
}

TEST_CASE("LightsNode custom cache key folds ids, shadow flags and spot light data") {
    const auto node = nativeLightsNodeConstructor().construct();
    auto plain = JSValue::object(nullptr);
    plain.set("id", 7);
    plain.set("castShadow", false);
    auto spot = JSValue::object(nullptr);
    spot.set("id", 3);
    spot.set("castShadow", true);
    spot.set("isSpotLight", true);
    spot.set("map", nullptr);
    spot.set("colorNode", nullptr);
    auto list = JSValue::array();
    list.set(JSValue(0), plain);
    list.set(JSValue(1), spot);
    node.get("setLights").call(node, {list});

    const auto key = node.get("customCacheKey").call(node, {});
    REQUIRE(key.type() == JSValue::Type::Number);
    // The same lights hash to the same key, and a changed shadow flag changes it.
    REQUIRE(node.get("customCacheKey").call(node, {}).toNumber() == Approx(key.toNumber()));
    spot.set("castShadow", false);
    REQUIRE(node.get("customCacheKey").call(node, {}).toNumber() != Approx(key.toNumber()));
}

TEST_CASE("LightsNode getHash sorts the light nodes by id and caches the result") {
    const auto node = nativeLightsNodeConstructor().construct();
    auto list = JSValue::array();
    list.set(JSValue(0), nodeLight(2, "b"));
    list.set(JSValue(1), nodeLight(1, "a"));
    node.get("setLights").call(node, {list});

    auto data = JSValue::object(nullptr);
    const auto builder = makeBuilder(data);
    // Sorted ascending by id, so the second entry comes first.
    REQUIRE(node.get("getHash").call(node, {builder}).strictEquals("lights-a,b"));
    // The hash is cached on the builder's node data.
    REQUIRE(data.get("lightNodesHash").strictEquals("lights-a,b"));
    REQUIRE(node.get("getHash").call(node, {builder}).strictEquals("lights-a,b"));
    // getLightNodes answers the same array through the same cache.
    const auto lightNodes = node.get("getLightNodes").call(node, {builder});
    REQUIRE(lightNodes.length() == 2);
    REQUIRE(lightNodes.get(JSValue(0)).get("id").toNumber() == Approx(1));
}

TEST_CASE("LightsNode module factory builds a node with the given lights") {
    const auto factory = nativeLightsFactory();
    REQUIRE(factory.isCallable());
    REQUIRE(factory.get("name").strictEquals("lights"));
    // The parameter has a default, so the wrapper's length is zero.
    REQUIRE(factory.get("length").strictEquals(0));

    auto list = JSValue::array();
    list.set(JSValue(0), JSValue::object(nullptr));
    const auto built = factory.call({}, {list});
    REQUIRE(built.get("type").strictEquals("LightsNode"));
    REQUIRE(built.get("getLights").call(built, {}).sameValue(list));

    // With no argument the source defaults to an empty array.
    const auto empty = factory.call({}, {});
    REQUIRE(empty.get("getLights").call(empty, {}).length() == 0);
}

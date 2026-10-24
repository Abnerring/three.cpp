#include "nodes/core/NodeFrame.h"
#include "core/JSWeakMap.h"
#include "catch.hpp"

namespace {
using Args = std::vector<JSValue>;
struct ClockReset { ~ClockReset() { NodeFrame::resetClock(); } };
JSValue nodeWithUpdate(const char* type, JSValue state, JSValue reference = {}) {
    auto node = JSValue::object();
    for (const char* method : {"getUpdateType", "getUpdateBeforeType", "getUpdateAfterType"})
        node.set(method, JSValue::function([](const JSValue&, const Args&, const Args& captures) {
            return captures[0];
        }, {type}, method, 1));
    node.set("updateReference", JSValue::function([](const JSValue& self, const Args&, const Args& captures) {
        return captures[0].isUndefined() ? self : captures[0];
    }, {reference}, "updateReference", 1));
    for (const char* method : {"update", "updateBefore", "updateAfter"})
        node.set(method, JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue {
            auto state = captures[0];
            state.set("calls", state.get("calls").toNumber() + 1);
            return state.get("result");
        }, {state}, method, 1));
    return node;
}
JSValue counter() {
    auto state = JSValue::object();
    state.set("calls", 0);
    return state;
}
}

TEST_CASE("NodeFrame has live state and independent weak update maps", "[nodes][nodeframe]") {
    NodeFrame frame;
    const auto state = frame.object();
    REQUIRE(state.keys() == std::vector<std::string>{"time", "deltaTime", "frameId", "renderId",
        "updateMap", "updateBeforeMap", "updateAfterMap", "renderer", "material", "camera", "object", "scene", "compute"});
    REQUIRE(frame.time.toNumber() == 0);
    REQUIRE(frame.frameId.toNumber() == 0);
    REQUIRE(frame.renderer.isNull());
    REQUIRE(frame.lastTime.isUndefined());
    REQUIRE(isNativeWeakMap(frame.updateMap.value()));
    REQUIRE_FALSE(frame.updateMap.strictEquals(frame.updateBeforeMap.value()));
    NodeFrame alias(frame);
    alias.frameId = "5";
    REQUIRE(frame.frameId.strictEquals("5"));
    NodeFrame other;
    other = frame;
    other.compute = state;
    REQUIRE(frame.compute.strictEquals(state));
    REQUIRE(other.object().sameValue(state));
}

TEST_CASE("NodeFrame clock preserves three first reads and two later reads", "[nodes][nodeframe]") {
    NodeFrame frame;
    ClockReset reset;
    auto clock = JSValue::object();
    auto samples = JSValue::array();
    for (double value : {100., 104., 107., 118., 120.})
        samples.set(std::to_string(samples.length()), value);
    clock.set("samples", samples);
    clock.set("cursor", 0);
    clock.set("now", JSValue::function([](const JSValue& self, const Args&, const Args&) {
        auto clock = self;
        const auto index = clock.get("cursor").toNumber();
        clock.set("cursor", index + 1);
        return clock.get("samples").get(std::to_string(static_cast<unsigned>(index)));
    }, {}, "now", 0));
    NodeFrame::setClock(clock);
    frame.update();
    REQUIRE(frame.frameId.toNumber() == 1);
    REQUIRE(frame.deltaTime.toNumber() == 0.004);
    REQUIRE(frame.lastTime.toNumber() == 107);
    REQUIRE(frame.time.toNumber() == 0.004);
    frame.update();
    REQUIRE(frame.frameId.toNumber() == 2);
    REQUIRE(frame.deltaTime.toNumber() == 0.011);
    REQUIRE(frame.time.toNumber() == 0.004 + 0.011);
    REQUIRE(clock.get("cursor").number() == 5);
    REQUIRE(frame.renderId.toNumber() == 0);
}

TEST_CASE("NodeFrame actually dispatches and retries only strict false", "[nodes][nodeframe]") {
    for (const char* method : {"updateBeforeNode", "updateNode", "updateAfterNode"}) {
        NodeFrame frame;
        auto state = counter();
        auto node = nodeWithUpdate("frame", state);
        const auto call = [&] { frame.object().get(method).call(frame.object(), {node}); };
        call();
        REQUIRE(state.get("calls").number() == 0);
        frame.frameId = 1;
        state.set("result", false);
        call(); call();
        REQUIRE(state.get("calls").number() == 2);
        state.set("result", 0);
        call(); call();
        REQUIRE(state.get("calls").number() == 3);
        frame.frameId = 2;
        call();
        REQUIRE(state.get("calls").number() == 4);
    }
}

TEST_CASE("NodeFrame shares reference gates but object updates stay ungated", "[nodes][nodeframe]") {
    NodeFrame frame;
    auto state = counter();
    const auto reference = JSValue::object();
    auto first = nodeWithUpdate("render", state, reference);
    auto second = nodeWithUpdate("render", state, reference);
    frame.renderId = 1;
    frame.updateNode(first);
    frame.updateNode(second);
    REQUIRE(state.get("calls").number() == 1);
    frame.renderId = 2;
    frame.updateNode(second);
    REQUIRE(state.get("calls").number() == 2);
    auto ungated = nodeWithUpdate("object", state, nullptr);
    frame.updateBeforeNode(ungated);
    frame.updateBeforeNode(ungated);
    REQUIRE(state.get("calls").number() == 4);
    REQUIRE_THROWS_AS(frame.updateNode(nodeWithUpdate("frame", state, nullptr)), JSTypeError);
}

TEST_CASE("NodeFrame weak maps do not retain nodes or their cyclic values", "[nodes][nodeframe][gc]") {
    NodeFrame frame;
    frame.frameId = 1;
    JSWeakValue weakNode, weakMaps;
    {
        auto state = counter();
        auto node = nodeWithUpdate("frame", state);
        frame.updateNode(node);
        auto map = JSWeakMap(frame.updateMap.value()).get(node);
        map.set("node", node);
        weakNode = JSWeakValue(node);
        weakMaps = JSWeakValue(map);
    }
    frame.object().collectGarbage();
    REQUIRE(weakNode.expired());
    REQUIRE(weakMaps.expired());
    JSWeakValue weakFrame;
    {
        NodeFrame cyclic;
        auto node = nodeWithUpdate("frame", counter());
        node.set("frame", cyclic.object());
        cyclic.frameId = 1;
        cyclic.updateNode(node);
        weakFrame = JSWeakValue(cyclic.object());
    }
    frame.object().collectGarbage();
    REQUIRE(weakFrame.expired());
}

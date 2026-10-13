#include "catch.hpp"
#include "parity/WebGLAnimationFixture.h"
#include <thread>

TEST_CASE("WebGLAnimation module closures retain context and loop then release their cycle", "[webgl-animation][gc]") {
    auto witness = JSValue::object();
    for (const auto* name : {"start", "stop", "setAnimationLoop", "setContext"}) {
        JSValue held;
        JSWeakValue context, loop, object;
        {
            WebGLAnimation animation;
            const auto nativeContext = JSValue::object();
            const auto callback = JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                const std::vector<JSValue>&) -> JSValue { return {}; });
            animation.setContext(nativeContext); animation.setAnimationLoop(callback);
            context = JSWeakValue(nativeContext); loop = JSWeakValue(callback); object = JSWeakValue(animation.object());
            held = animation.object().get(name);
        }
        witness.collectGarbage();
        REQUIRE(object.expired()); REQUIRE_FALSE(context.expired()); REQUIRE_FALSE(loop.expired());
        held = {}; witness.collectGarbage(); REQUIRE(context.expired()); REQUIRE(loop.expired());
    }
}
TEST_CASE("WebGLAnimation native access rejects foreign threads before copying handles", "[webgl-animation][thread]") {
    WebGLAnimation animation;
    const auto foreign = JSValue::object();
    unsigned rejected = 0;
    std::thread worker([&] {
        try { static_cast<void>(animation.object()); } catch (const std::logic_error&) { ++rejected; }
        try { WebGLAnimation copy(animation); } catch (const std::logic_error&) { ++rejected; }
        try { WebGLAnimation moved(std::move(animation)); } catch (const std::logic_error&) { ++rejected; }
        try { animation.start(); } catch (const std::logic_error&) { ++rejected; }
        try { WebGLAnimation local; local.setContext(foreign); } catch (const std::logic_error&) { ++rejected; }
    });
    worker.join(); REQUIRE(rejected == 5); REQUIRE(animation.object().isObject());
}

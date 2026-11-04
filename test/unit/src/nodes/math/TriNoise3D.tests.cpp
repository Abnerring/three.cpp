#include "nodes/math/TriNoise3D.h"

#include "catch.hpp"
#include "nodes/tsl/TSLBase.h"

TEST_CASE("TriNoise3D exports the typed noise function") {
    const auto module = nativeTriNoise3DModule();
    const auto noise = nativeTriNoise3D();
    REQUIRE(module.get("triNoise3D").sameValue(noise));
    REQUIRE(noise.get("isFn").strictEquals(true));
    REQUIRE(noise.get("isNode").strictEquals(true));
    REQUIRE(noise.get("length").isCallable());
    const auto position = nativeTSLBaseBinding("vec3").call({}, {0, 1, 2});
    const auto speed = nativeTSLBaseBinding("float").call({}, {1});
    const auto time = nativeTSLBaseBinding("float").call({}, {0.5});
    const auto call = noise.call({}, {position, speed, time});
    REQUIRE(call.get("type").strictEquals("VarNode"));
    REQUIRE(call.get("node").get("rawInputs").get("length").strictEquals(3));
}


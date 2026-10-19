#include "catch.hpp"
#include "audio/PositionalAudio.h"
#include "core/JSValue.h"
#include <vector>

namespace {
JSValue node() {
    auto created = JSValue::object();
    auto param = JSValue::object();
    param.set("value", 1);
    created.set("gain", param);
    for (const char* name : {"connect", "disconnect"})
        created.set(name, JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, name, 1));
    for (const char* name : {"start", "stop"})
        created.set(name, JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, name, 1));
    return created;
}
JSValue panner() {
    auto created = node();
    created.set("panningModel", "");
    created.set("refDistance", 1);
    created.set("rolloffFactor", 1);
    created.set("maxDistance", 10000);
    created.set("distanceModel", "inverse");
    // The older positional path uses these two instead of the parameter ramps.
    for (const char* name : {"setPosition", "setOrientation"})
        created.set(name, JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, name, 3));
    return created;
}
}

// The panner is the output of a positional source, and its parameters are reachable under the upstream names.
TEST_CASE("PositionalAudio owns a panner and its parameters") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto context = JSValue::object();
    context.set("currentTime", 4);
    context.set("destination", node());
    context.set("createGain", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return node(); }, {}, "createGain", 0));
    context.set("createPanner", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return panner(); }, {}, "createPanner", 0));
    auto listener = JSValue::object();
    listener.set("context", context);
    listener.set("timeDelta", 0.5);
    listener.set("getInput", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return node(); }, {}, "getInput", 0));

    PositionalAudio audio(listener);
    REQUIRE(audio.getOutput().sameValue(audio.panner));
    REQUIRE(audio.panner.get("panningModel").toString() == "HRTF");
    audio.setRefDistance(2);
    REQUIRE(audio.getRefDistance() == 2.0);
    audio.setRolloffFactor(3);
    REQUIRE(audio.getRolloffFactor() == 3.0);
    audio.setMaxDistance(40);
    REQUIRE(audio.getMaxDistance() == 40.0);
    audio.setDirectionalCone(30, 60, 0.5);
    REQUIRE(audio.panner.get("coneOuterAngle").toNumber() == 60.0);

    // A node source has no playback control, so the transform is pushed even when it is not playing.
    audio.setNodeSource(node());
    audio.position.set(1.0, 2.0, 3.0);
    audio.updateMatrix();
    audio.updateMatrixWorld(true);
    REQUIRE(audio.panner.get("setPosition").isCallable());
}
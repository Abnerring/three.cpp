#include "catch.hpp"
#include "audio/Audio.h"
#include "core/JSValue.h"
#include <vector>

namespace {
JSValue object(const std::vector<JSValue>& names, const std::vector<JSValue>&) {
    auto node = JSValue::object();
    auto param = JSValue::object();
    param.set("value", 1);
    node.set("gain", param);
    node.set("connect", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "connect", 1));
    node.set("disconnect", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "disconnect", 0));
    node.set("start", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "start", 3));
    node.set("stop", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "stop", 1));
    return node;
}
}

// The state machine is the part worth testing without a device: volume and rate round trip to the graph,
// and progress accumulates from the context clock scaled by the rate, wrapping when looping.
TEST_CASE("Audio tracks playback progress against the context clock") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto context = JSValue::object();
    context.set("currentTime", 10);
    context.set("createGain", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return object({}, {}); }, {}, "createGain", 0));
    context.set("createBufferSource", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return object({}, {}); }, {}, "createBufferSource", 0));
    auto listener = JSValue::object();
    listener.set("context", context);
    listener.set("getInput", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return JSValue::object(); }, {}, "getInput", 0));

    Audio audio(listener);
    REQUIRE(audio.isPlaying == false);
    REQUIRE(audio.getVolume() == 1.0);
    audio.setVolume(0.5);
    REQUIRE(audio.getVolume() == 0.5);
    audio.setPlaybackRate(2);
    REQUIRE(audio.getPlaybackRate() == 2.0);
    REQUIRE(audio.setLoop(true).getLoop());
    // A looping source needs a duration to wrap by, either its own or the buffer one.
    auto buffer = JSValue::object();
    buffer.set("duration", 100);
    audio.setBuffer(buffer);

    audio.play();
    REQUIRE(audio.isPlaying);
    REQUIRE(audio.startedAt == 10.0);

    // Three seconds of context time at twice the rate is six seconds of progress.
    context.set("currentTime", 13);
    audio.pause();
    REQUIRE(audio.isPlaying == false);
    REQUIRE(audio.progress == 6.0);

    audio.play();
    audio.stop();
    REQUIRE(audio.progress == 0.0);
    REQUIRE(audio.isPlaying == false);
}
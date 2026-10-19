#include "catch.hpp"
#include "audio/AudioAnalyser.h"
#include "core/JSValue.h"
#include <vector>

namespace {
int analyserCount = 0;

// The host analyser reports a fixed bin count and fills the caller array with the same value for every bin,
// which makes the average checkable without a device.
JSValue makeAnalyser() {
    analyserCount += 1;
    auto node = JSValue::object();
    node.set("fftSize", 0);
    node.set("frequencyBinCount", JSValue(4));
    node.set("connect", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "connect", 1));
    node.set("getByteFrequencyData", JSValue::function([](const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
        auto data = args.at(0);
        const auto bins = self.get("frequencyBinCount").toNumber();
        for (double i = 0; i < bins; i += 1) data.set(i, JSValue(2));
        return data;
    }, {}, "getByteFrequencyData", 1));
    return node;
}

JSValue makeOutput() {
    auto output = JSValue::object();
    output.set("connect", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "connect", 1));
    return output;
}
}

// The analyser node is created from the audio context, its fft size is set before the bin count is read, and
// the data array is sized from that count and reused for every read.
TEST_CASE("AudioAnalyser sizes its data from the analyser node") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    analyserCount = 0;
    auto context = JSValue::object();
    context.set("createAnalyser", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return makeAnalyser(); }, {}, "createAnalyser", 0));
    auto audio = JSValue::object();
    audio.set("context", context);
    audio.set("getOutput", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return makeOutput(); }, {}, "getOutput", 0));

    AudioAnalyser analyser(audio, 1024);
    REQUIRE(analyserCount == 1);
    // The fft size reached the node before the bin count was read.
    REQUIRE(analyser.object().get("analyser").get("fftSize").toNumber() == 1024.0);
    const auto data = analyser.object().get("data");
    REQUIRE(data.get("length").toNumber() == 4.0);
    REQUIRE(analyser.getAverageFrequency() == 2.0);
}
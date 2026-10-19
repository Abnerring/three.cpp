#include "catch.hpp"
#include "audio/AudioContext.h"
#include "core/JSValue.h"
#include "loaders/AudioLoader.h"
#include "loaders/FileLoader.h"
#include <vector>

namespace {
double receivedDuration = -1;

JSValue transport(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    static_cast<void>(args);
    auto response = JSValue::object();
    response.set("status", 200);
    response.set("statusText", "OK");
    response.set("url", "");
    auto headers = JSValue::object();
    headers.set("get", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return JSValue(nullptr); }, {}, "get", 1));
    response.set("headers", headers);
    // A tiny body: the loader only needs something to hand to the decoder.
    response.set("body", "RIFF");
    return response;
}

JSValue decoded(const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    // The context answers inside the call, which is what a synchronous host does.
    auto audioBuffer = JSValue::object();
    audioBuffer.set("duration", 12);
    const auto callback = args.at(1);
    callback.call(self, {audioBuffer});
    return audioBuffer;
}
}

// The bytes come through the file loader and the decode through the context, with the decoded buffer
// reaching the caller.
TEST_CASE("AudioLoader decodes through the audio context") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    receivedDuration = -1;
    FileLoader::setTransport(JSValue::function(transport, {}, "transport", 1));
    auto context = JSValue::object();
    context.set("decodeAudioData", JSValue::function(decoded, {}, "decodeAudioData", 2));
    AudioContext::setContext(context);

    AudioLoader loader;
    loader.load("sound.ogg", JSValue::function([](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
        receivedDuration = args.at(0).get("duration").toNumber();
        return JSValue();
    }, {}, "onLoad", 1));
    REQUIRE(receivedDuration == 12.0);
    AudioContext::setContext(JSValue());
    FileLoader::setTransport(JSValue());
}
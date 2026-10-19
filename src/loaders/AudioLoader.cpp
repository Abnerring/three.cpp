#include "loaders/AudioLoader.h"

#include "audio/AudioContext.h"
#include "loaders/FileLoader.h"
#include "loaders/LoadingManager.h"
#include <stdexcept>

namespace {
// The decode callback needs the loader state, and the JS function has to be captureless, so the pending
// values live in a slot for the duration of the call.
struct Pending {
    JSValue onLoad, onError, decodeUrl, url;
    JSValue manager;
};
Pending& pending() {
    static thread_local Pending value;
    return value;
}

JSValue decoded(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    auto& state = pending();
    const auto audioBuffer = args.empty() ? JSValue() : args[0];
    state.manager.get("itemEnd").call(state.manager, {state.decodeUrl});
    if (!state.onLoad.isUndefined()) state.onLoad.call(JSValue(), {audioBuffer});
    return audioBuffer;
}
}

AudioLoader::AudioLoader(JSValue manager) : Loader(std::move(manager)) {}

JSValue AudioLoader::load(JSValue url, JSValue onLoad, JSValue onProgress, JSValue onError) {
    auto self = object_;
    auto manager = LoadingManager::fromObject(self.get("manager"));
    auto& state = pending();
    state.onLoad = onLoad;
    state.onError = onError;
    state.url = url;
    state.decodeUrl = JSValue(url.toString() + "#decode");
    state.manager = manager.object();

    FileLoader loader(self.get("manager"));
    loader.setResponseType(JSValue("arraybuffer"));
    const auto path = self.get("path");
    if (!path.isUndefined()) loader.setPath(path);
    const auto headers = self.get("requestHeader");
    if (!headers.isUndefined()) loader.setRequestHeader(headers);
    const auto credentials = self.get("withCredentials");
    if (!credentials.isUndefined()) loader.setWithCredentials(credentials);

    const auto onBytes = JSValue::function([](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
        auto& current = pending();
        try {
            // decodeAudioData detaches the buffer, so a copy goes to it, as upstream does.
            const auto buffer = args.empty() ? JSValue() : args[0];
            auto copied = buffer;
            const auto slice = buffer.get("slice");
            if (slice.isCallable()) copied = slice.call(buffer, {JSValue(0)});
            const auto context = AudioContext::getContext();
            current.manager.get("itemStart").call(current.manager, {current.decodeUrl});
            context.get("decodeAudioData").call(context, {copied, JSValue::function(decoded, {}, "decoded", 1)});
        } catch (const std::exception& failure) {
            current.manager.get("itemError").call(current.manager, {current.url});
            if (!current.onError.isUndefined()) current.onError.call(JSValue(), {JSValue(failure.what())});
        }
        return JSValue();
    }, {}, "onBytes", 1);
    return loader.load(url, onBytes, onProgress, onError);
}
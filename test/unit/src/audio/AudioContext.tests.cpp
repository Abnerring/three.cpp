#include "catch.hpp"
#include "audio/AudioContext.h"
#include "core/JSValue.h"
#include <stdexcept>

// One shared context, installed by the host. Asking for it before that is refused by name rather than
// returning an empty object that would look like a working context.
TEST_CASE("AudioContext hands back the installed context") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    AudioContext::setContext(JSValue());
    REQUIRE_THROWS_AS(AudioContext::getContext(), std::runtime_error);

    auto context = JSValue::object();
    context.set("sampleRate", 48000);
    AudioContext::setContext(context);
    const auto first = AudioContext::getContext();
    const auto second = AudioContext::getContext();
    REQUIRE(first.sameValue(second));
    REQUIRE(first.get("sampleRate").toNumber() == 48000.0);

    // Replaceable at any time, which is what setContext is for.
    auto replacement = JSValue::object();
    replacement.set("sampleRate", 44100);
    AudioContext::setContext(replacement);
    REQUIRE(AudioContext::getContext().get("sampleRate").toNumber() == 44100.0);
    AudioContext::setContext(JSValue());
}
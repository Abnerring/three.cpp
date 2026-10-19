#include "catch.hpp"
#include "audio/AudioContext.h"
#include "audio/AudioListener.h"
#include "core/JSValue.h"
#include <vector>

namespace {
JSValue node() {
    auto created = JSValue::object();
    auto param = JSValue::object();
    param.set("value", 1);
    param.set("setTargetAtTime", JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, "setTargetAtTime", 3));
    created.set("gain", param);
    for (const char* name : {"connect", "disconnect"})
        created.set(name, JSValue::function([](const JSValue& self, const std::vector<JSValue>&, const std::vector<JSValue>&) { return self; }, {}, name, 1));
    return created;
}
}

// The listener owns the master gain built from the shared context, and routes it through one filter while
// reporting what it routed through.
TEST_CASE("AudioListener owns the master gain and one filter") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto context = JSValue::object();
    context.set("currentTime", 5);
    context.set("destination", JSValue::object());
    context.set("createGain", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return node(); }, {}, "createGain", 0));
    AudioContext::setContext(context);

    AudioListener listener;
    REQUIRE(listener.type == "AudioListener");
    REQUIRE(!listener.getInput().isUndefined());
    REQUIRE(listener.getFilter().isNull());
    REQUIRE(listener.getMasterVolume().toNumber() == 1.0);

    auto filter = node();
    listener.setFilter(filter);
    REQUIRE(listener.getFilter().sameValue(filter));
    listener.removeFilter();
    REQUIRE(listener.getFilter().isNull());
    AudioContext::setContext(JSValue());
}
#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/FileLoader.h"
#include "loaders/XYZLoader.h"
#include <string>
#include <vector>

// The load path runs through the injected FileLoader transport, so these cases use the same shape
// docs/HANDOFF.md item 10 freezes. The manager is a plain object with the four methods the loader
// calls, which makes the itemError notification observable.
namespace {
using Args = std::vector<JSValue>;
struct State {
    int loaded = 0;
    int errors = 0;
    int itemErrors = 0;
    std::string text;
    std::string message;
    double positionCount = -1;
};
State state;
JSValue managerFn(const JSValue&, const Args& args, const Args& captures) {
    const auto name = captures[0].string();
    if (name == "itemError") state.itemErrors += 1;
    if (name == "resolveURL") return args.empty() ? JSValue{} : args[0];
    return {};
}
JSValue transportFn(const JSValue&, const Args&, const Args&) {
    auto response = JSValue::object();
    response.set("status", 200.0);
    response.set("statusText", std::string("OK"));
    response.set("url", std::string("https://host/cloud.xyz"));
    auto headers = JSValue::object();
    headers.set("get", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return JSValue{}; }, {}, "get", 1));
    response.set("headers", headers);
    response.set("body", state.text);
    return response;
}
JSValue onLoadFn(const JSValue&, const Args& args, const Args&) {
    state.loaded += 1;
    if (!args.empty()) state.positionCount = args[0].get("attributes").get("position").get("count").toNumber();
    return {};
}
JSValue onErrorFn(const JSValue&, const Args& args, const Args&) {
    state.errors += 1;
    state.message = args.empty() ? std::string{} : args[0].get("message").toString();
    return {};
}
JSValue makeManager() {
    auto manager = JSValue::object();
    for (const auto& name : {"itemStart", "itemEnd", "itemError", "resolveURL"}) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = descriptor.writable = true;
        descriptor.value = JSValue::function(managerFn, {name}, name, 1);
        manager.defineProperty(name, descriptor);
    }
    return manager;
}
}

TEST_CASE("XYZLoader loads text through the transport and parses it", "[xyzloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    state = State{};
    state.text = "1 2 3\n4 5 6\n7 8 9\n";
    FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
    XYZLoader loader(makeManager());
    loader.load("cloud.xyz", JSValue::function(onLoadFn, {}, "onLoad", 1), JSValue{}, JSValue::function(onErrorFn, {}, "onError", 1));
    REQUIRE(state.loaded == 1);
    REQUIRE(state.errors == 0);
    REQUIRE(state.positionCount == 3);
    REQUIRE(state.itemErrors == 0);
    FileLoader::setTransport(JSValue{});
}

TEST_CASE("XYZLoader loads a document it cannot read as an empty geometry", "[xyzloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    state = State{};
    // Five values on a line is neither the XYZ nor the XYZRGB layout, so the line is ignored and
    // the result is an empty geometry rather than an error: the pinned parse cannot throw for any
    // input, because every branch either skips a line or pushes a triple. The onError and
    // manager.itemError path therefore exists for a failure raised by the parse of a future format,
    // and is documented rather than exercised by a document that cannot reach it.
    state.text = "not a number at all\n";
    FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
    XYZLoader loader(makeManager());
    loader.load("odd.xyz", JSValue::function(onLoadFn, {}, "onLoad", 1), JSValue{}, JSValue::function(onErrorFn, {}, "onError", 1));
    REQUIRE(state.loaded == 1);
    REQUIRE(state.errors == 0);
    REQUIRE(state.positionCount == 0);
    REQUIRE(state.itemErrors == 0);
    FileLoader::setTransport(JSValue{});
}
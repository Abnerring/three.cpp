#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/Cache.h"
#include "loaders/FileLoader.h"
#include <string>
#include <vector>

// Drives the ported FileLoader through the same injected transport the diagnostics program uses:
// no network, no fetch, and the request/response shape frozen in docs/HANDOFF.md item 10.
//
// Two facts about the pinned module shape these tests. Cache defines enabled as false, so the
// cache short-circuit only exists when enabled is set BEFORE the first load - enabling it
// afterwards still refetches once and only then stores the entry. And JSValue::NativeFunction is
// a plain function type rather than a std::function, so the callbacks below are non-capturing and
// keep their state in a file-scope struct.
namespace {
using Args = std::vector<JSValue>;
struct State {
    int transportCalls = 0;
    int loads = 0;
    std::string received;
    double status = 200;
    std::string statusText = "OK";
    std::string body = "hello world";
    std::string contentLength = "11";
};
State state;
void reset() {
    state.transportCalls = 0;
    state.loads = 0;
    state.received.clear();
    state.status = 200;
    state.statusText = "OK";
    state.body = "hello world";
    state.contentLength = "11";
}
JSValue headerFn(const JSValue&, const Args& args, const Args&) {
    if (args.empty()) return {};
    return args[0].toString() == "Content-Length" ? JSValue(state.contentLength) : JSValue{};
}
JSValue transportFn(const JSValue&, const Args&, const Args&) {
    state.transportCalls += 1;
    auto response = JSValue::object();
    response.set("status", state.status);
    response.set("statusText", state.statusText);
    auto headers = JSValue::object();
    headers.set("get", JSValue::function(headerFn, {}, "get", 1));
    response.set("headers", headers);
    response.set("body", state.body);
    return response;
}
JSValue onLoadFn(const JSValue&, const Args& args, const Args&) {
    state.loads += 1;
    state.received = args.empty() ? std::string{} : args[0].toString();
    return {};
}
JSValue errorFn(const JSValue&, const Args&, const Args&) { return {}; }
// Each case starts from a live realm because Cache mutations require one.
void prepare() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    Cache.clear();
    Cache.enabled = false;
    FileLoader::setTransport(JSValue{});
}
}

TEST_CASE("FileLoader loads through the injected transport", "[fileloader]") {
    prepare();
    reset();
    FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
    FileLoader loader;
    loader.load("asset.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    INFO("loads=" << state.loads << " calls=" << state.transportCalls << " received=" << state.received);
    REQUIRE(state.loads == 1);
    REQUIRE(state.received == "hello world");
    REQUIRE(state.transportCalls == 1);
    // With the cache disabled, like the pinned default, a second load of the same url refetches.
    loader.load("asset.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    INFO("after second: loads=" << state.loads << " calls=" << state.transportCalls);
    REQUIRE(state.loads == 2);
    REQUIRE(state.transportCalls == 2);
    FileLoader::setTransport(JSValue{});
}

TEST_CASE("FileLoader reuses the cache when it is enabled before the load", "[fileloader]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    Cache.clear();
    Cache.enabled = true;
    reset();
    state.body = "cached body";
    FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
    FileLoader loader;
    loader.load("cached.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    INFO("first: loads=" << state.loads << " calls=" << state.transportCalls);
    REQUIRE(state.transportCalls == 1);
    REQUIRE(state.loads == 1);
    // The entry is stored by the first load, so the cache answers the second one.
    loader.load("cached.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    INFO("second: loads=" << state.loads << " calls=" << state.transportCalls << " received=" << state.received);
    REQUIRE(state.loads == 2);
    REQUIRE(state.received == "cached body");
    REQUIRE(state.transportCalls == 1);
    // A different url goes back to the transport.
    loader.load("other.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    REQUIRE(state.transportCalls == 2);
    Cache.clear();
    Cache.enabled = false;
    FileLoader::setTransport(JSValue{});
}

TEST_CASE("FileLoader reports failures the way the pinned source does", "[fileloader]") {
    prepare();
    reset();
    FileLoader loader;
    REQUIRE_THROWS_AS(loader.load("missing.txt", JSValue::function(onLoadFn, {}, "onLoad", 1)),
        JSTypeError);
    state.status = 404;
    state.statusText = "Not Found";
    FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
    try {
        loader.load("gone.txt", JSValue::function(onLoadFn, {}, "onLoad", 1));
        REQUIRE(false);
    } catch (const JSTypeError& error) {
        INFO("message: " << error.what());
        REQUIRE(std::string(error.what()).find("responded with 404: Not Found") != std::string::npos);
    }
    // A status of 0 counts as success, like the pinned non-http protocol case.
    state.status = 0;
    state.statusText = "";
    state.body = "zero status";
    loader.load("zero.txt", JSValue::function(onLoadFn, {}, "onLoad", 1),
        JSValue::function(errorFn, {}, "onProgress", 1), JSValue::function(errorFn, {}, "onError", 1));
    INFO("zero status received=" << state.received);
    REQUIRE(state.received == "zero status");
    // blob and document have no native counterpart, so they fail loudly.
    state.status = 200;
    state.body = "{}";
    loader.setResponseType("blob");
    REQUIRE_THROWS_AS(loader.load("blob.txt", JSValue::function(onLoadFn, {}, "onLoad", 1)), JSTypeError);
    loader.setResponseType("document");
    REQUIRE_THROWS_AS(loader.load("doc.txt", JSValue::function(onLoadFn, {}, "onLoad", 1)), JSTypeError);
    FileLoader::setTransport(JSValue{});
}
#include "core/EventDispatcher.h"
#include "catch.hpp"
#include "core/Layers.h"
#include <limits>

TEST_CASE("EventDispatcher listener identity and mutation snapshot", "[core]") {
    EventDispatcher dispatcher;
    std::vector<int> called;
    EventDispatcher::Listener second, added;
    auto first = dispatcher.addEventListener("event", [&](Event& event) {
        REQUIRE(event.target == &dispatcher);
        called.push_back(1);
        dispatcher.removeEventListener("event", second);
        if (!added)
            added = dispatcher.addEventListener("event", [&](Event&) { called.push_back(3); });
    });
    second = dispatcher.addEventListener("event", [&](Event&) { called.push_back(2); });
    dispatcher.addEventListener("event", second);
    REQUIRE(dispatcher.hasEventListener("event", second));
    Event event{"event"};
    dispatcher.dispatchEvent(event);
    REQUIRE(event.target == nullptr);
    REQUIRE(called == std::vector<int>{1, 2});
    REQUIRE_FALSE(dispatcher.hasEventListener("event", second));
    called.clear();
    dispatcher.dispatchEvent(event);
    REQUIRE(called == std::vector<int>{1, 3});
    dispatcher.removeEventListener("event", first);
    dispatcher.removeEventListener("event", added);
    dispatcher.dispatchEvent(Event{"missing"});
}

TEST_CASE("Layers implements all 32 bits and JavaScript shift counts", "[core]") {
    Layers a, b;
    REQUIRE(a.mask == 1);
    a.set(31);
    REQUIRE(a.mask == 0x80000000u);
    REQUIRE(a.isEnabled(31));
    REQUIRE_FALSE(a.test(b));
    a.enable(0);
    REQUIRE(a.test(b));
    a.toggle(31);
    REQUIRE(a.mask == 1);
    a.disable(0);
    REQUIRE(a.mask == 0);
    a.enableAll();
    REQUIRE(a.mask == -1);
    a.disableAll();
    REQUIRE(a.mask == 0);
    a.set(32);
    REQUIRE(a.mask == 1);
    a.set(-1);
    REQUIRE(a.mask == 0x80000000u);
}

TEST_CASE("JsonValue copies nested values and emits valid escaped JSON", "[core]") {
    JsonValue original =
        JsonValue::Object{{"text", "a\n\"b\\c"}, {"array", JsonValue::Array{1, true, nullptr}}};
    auto copied = original;
    copied["array"][std::size_t{0}] = 2;
    REQUIRE(original.at("array")[std::size_t{0}] == JsonValue(1));
    REQUIRE(original.dump() == "{\"text\":\"a\\n\\\"b\\\\c\",\"array\":[1,true,null]}");
    JsonValue nonfinite = JsonValue::Array{std::numeric_limits<double>::infinity(), -0.0};
    REQUIRE(nonfinite.dump() == "[null,0]");
    REQUIRE(nonfinite.copyJSON()[std::size_t{0}].isNull());
}

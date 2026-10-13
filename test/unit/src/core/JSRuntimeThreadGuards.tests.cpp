#include "catch.hpp"
#include "core/JSValue.h"
#include <array>
#include <atomic>
#include <thread>
TEST_CASE("Runtime property reads reject foreign receivers before copying",
          "[runtime-thread-guards]") {
    auto object = JSValue::object();
    object.set("value", 5);
    const auto symbol = JSValue::symbol("key");
    const auto function =
        JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                              const std::vector<JSValue>&) -> JSValue { return {}; });
    std::array<bool, 7> rejected{};
    std::thread worker([&] {
        const auto attempt = [&](unsigned index, auto body) {
            try {
                body();
            } catch (const JSTypeError&) {
                rejected[index] = true;
            }
        };
        const auto localSymbol = JSValue::symbol("local");
        auto local = JSValue::object();
        attempt(0, [&] { object.get("value"); });
        attempt(1, [&] { function.get("name"); });
        attempt(2, [&] { local.get("value", object); });
        attempt(3, [&] { JSValue(1).get("value", object); });
        attempt(4, [&] { object.get(localSymbol); });
        attempt(5, [&] { local.get(localSymbol, object); });
        attempt(6, [&] { local.get(symbol); });
    });
    worker.join();
    for (bool value : rejected)
        REQUIRE(value);
}
TEST_CASE("Runtime calls validate function receiver and every argument before rooting",
          "[runtime-thread-guards]") {
    auto foreign = JSValue::object();
    std::atomic<unsigned> calls = 0;
    auto foreignFunction =
        JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                              const std::vector<JSValue>&) -> JSValue { return 9; });
    const std::vector<JSValue> earlyArgument{foreign, 1}, lateArgument{1, foreign};
    std::array<bool, 5> rejected{};
    std::thread worker([&] {
        auto counter = JSValue::hostObject(std::shared_ptr<void>(&calls, [](void*) {}), &calls,
                                           "audit.counter");
        auto local = JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&,
                const std::vector<JSValue>& captured) -> JSValue {
                ++*std::static_pointer_cast<std::atomic<unsigned>>(
                    captured[0].hostPayload("audit.counter"));
                return {};
            },
            {counter});
        const auto attempt = [&](unsigned index, auto body) {
            try {
                body();
            } catch (const JSTypeError&) {
                rejected[index] = true;
            }
        };
        attempt(0, [&] { foreignFunction.call(); });
        attempt(1, [&] { local.call(foreign); });
        attempt(2, [&] { local.call({}, earlyArgument); });
        attempt(3, [&] { local.call({}, lateArgument); });
        attempt(4, [&] { foreignFunction.call(foreign, lateArgument); });
    });
    worker.join();
    for (bool value : rejected)
        REQUIRE(value);
    REQUIRE(calls == 0);
}

#include "catch.hpp"
#include "renderers/shaders/UniformsLib.h"
#include <array>
#include <exception>
#include <thread>

TEST_CASE("Module facade mutations require an existing realm and preserve one authoritative export",
          "[runtime][modules][gc]") {
    bool initiallyEmpty = false, noAllocation = false, retained = false, released = false,
         reset = false, objectArgument = false;
    std::array<bool, 7> rejected{};
    std::exception_ptr failure;
    std::thread worker([&] {
        try {
            initiallyEmpty = !JSValue::hasActiveHeap();
            auto reject = [&](unsigned index, auto action) {
                try { action(); }
                catch (const JSTypeError&) { rejected[index] = true; }
            };
            JSPropertyDescriptor descriptor;
            descriptor.value = 7;
            descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
            reject(0, [&] { UniformsLib.set("marker", 7); });
            reject(1, [&] { UniformsLib.erase("common"); });
            reject(2, [&] { UniformsLib.defineOwnProperty("marker", descriptor); });
            reject(3, [&] { UniformsLib.defineProperty("marker", descriptor); });
            reject(4, [&] { UniformsLib.setPrototype(nullptr); });
            reject(5, [&] { UniformsLib.preventExtensions(); });
            reject(6, [&] { UniformsLib.reflectSet("marker", 7, {}); });
            noAllocation = !JSValue::hasActiveHeap();
            JSWeakValue previous;
            {
                const JSValue library = UniformsLib;
                previous = JSWeakValue(library);
                UniformsLib.set("marker", 7);
                UniformsLib.defineProperty("other", descriptor);
                retained = UniformsLib.object().sameValue(library)
                        && library.get("marker").strictEquals(7)
                        && library.get("other").strictEquals(7);
                UniformsLib.erase("other");
                retained = retained && !library.hasOwn("other");
            }
            released = previous.expired() && !JSValue::hasActiveHeap();
            {
                const JSValue fresh = UniformsLib;
                reset = !fresh.hasOwn("marker") && !fresh.hasOwn("other");
            }
            {
                const auto value = JSValue::object();
                UniformsLib.set("objectRoot", value);
                objectArgument = UniformsLib.get("objectRoot").sameValue(value);
            }
        } catch (...) { failure = std::current_exception(); }
    });
    worker.join();
    if (failure) std::rethrow_exception(failure);
    REQUIRE(initiallyEmpty);
    for (const auto value : rejected) REQUIRE(value);
    REQUIRE(noAllocation);
    REQUIRE(retained);
    REQUIRE(released);
    REQUIRE(reset);
    REQUIRE(objectArgument);
}

TEST_CASE("Module facade validates foreign handles before checking or allocating a realm",
          "[runtime][modules][thread]") {
    const auto foreign = JSValue::object();
    bool rejected = false, stayedEmpty = false;
    std::exception_ptr failure;
    std::thread worker([&] {
        try {
            try { UniformsLib.set("foreign", foreign); }
            catch (const std::logic_error&) { rejected = true; }
            stayedEmpty = !JSValue::hasActiveHeap();
        } catch (...) { failure = std::current_exception(); }
    });
    worker.join();
    if (failure) std::rethrow_exception(failure);
    REQUIRE(rejected);
    REQUIRE(stayedEmpty);
}

#include "core/JSNumber.h"
#include "catch.hpp"
#include "core/JSString.h"
#include <atomic>
#include <cmath>
#include <exception>
#include <thread>
namespace {
void own(JSValue object, const JSValue& key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
template <class Body> void isolated(Body body) {
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            body();
        } catch (...) {
            error = std::current_exception();
        }
    });
    worker.join();
    if (error)
        std::rethrow_exception(error);
}
} // namespace
TEST_CASE("NumberData preserves brand independently of its public prototype", "[number]") {
    auto realm = JSValue::object(), prototype = nativeNumberPrototype();
    const auto valueOf = prototype.get("valueOf"), fixed = prototype.get("toFixed");
    auto object = createNativeNumberObject(-0.0);
    REQUIRE(isNativeNumberObject(object));
    REQUIRE(object.getPrototype().sameValue(prototype));
    REQUIRE(object.ownPropertyKeys().empty());
    REQUIRE(std::signbit(valueOf.call(object).number()));
    object.setPrototype(nullptr);
    REQUIRE(std::signbit(valueOf.call(object).number()));
    REQUIRE(fixed.call(object, {3}).string() == "0.000");
    REQUIRE_THROWS_AS(valueOf.call(JSValue::object(prototype)), JSTypeError);
    REQUIRE_THROWS_AS(fixed.call(createNativeStringObject("1.5"), {2}), JSTypeError);
    REQUIRE_THROWS_AS(createNativeNumberObject("1.5"), JSTypeError);
    REQUIRE_THROWS_AS(createNativeNumberObject(JSValue::object()), JSTypeError);
    REQUIRE(valueOf.call(prototype).sameValue(0));
    REQUIRE(realm.get("toString").call(object).string() == "[object Number]");
    const auto tag = JSValue::wellKnownSymbol("toStringTag");
    own(prototype, tag, "CustomNumber");
    REQUIRE(realm.get("toString").call(1.25).string() == "[object CustomNumber]");
    prototype.erase(tag);
}
TEST_CASE("Number methods remain mutable inherited methods on primitive reads", "[number]") {
    const auto realm = JSValue::object();
    auto prototype = nativeNumberPrototype();
    const auto original = *prototype.getOwnPropertyDescriptor("toFixed");
    auto replacement =
        JSValue::function(+[](const JSValue& self, const std::vector<JSValue>&,
                              const std::vector<JSValue>&) -> JSValue { return self; });
    prototype.set("toFixed", replacement);
    REQUIRE(JSValue(2.5).get("toFixed").sameValue(replacement));
    REQUIRE(JSValue(2.5).get("toFixed").call(2.5).number() == 2.5);
    prototype.erase("toFixed");
    REQUIRE(JSValue(2.5).get("toFixed").isUndefined());
    prototype.defineProperty("toFixed", original);
    REQUIRE(JSValue(2.5).get("toFixed").call(2.5, {0}).string() == "3");
    REQUIRE(JSValue(-2.5).get("toFixed").call(-2.5, {0}).string() == "-3");
    auto boxed = createNativeNumberObject(2.5);
    boxed.set("toString", replacement);
    REQUIRE(boxed.toNumber() == 2.5);
    boxed.set("valueOf",
              JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                                    const std::vector<JSValue>&) -> JSValue { return 8; }));
    REQUIRE(boxed.toNumber() == 8);
    REQUIRE(prototype.get("toFixed").call(boxed, {1}).string() == "2.5");
    const auto values = JSValue::array().get("values");
    own(prototype, "length", 1);
    own(prototype, "0", 12);
    const auto iterator = values.call(2.5);
    const auto result = iterator.get("next").call(iterator);
    REQUIRE_FALSE(result.get("done").toBoolean());
    REQUIRE(result.get("value").number() == 12);
    prototype.erase("length");
    prototype.erase("0");
}
TEST_CASE("Number toString declares its unsupported finite radix boundary", "[number]") {
    const auto method = nativeNumberPrototype().get("toString");
    for (unsigned radix = 2; radix <= 36; ++radix) {
        if (radix == 10)
            REQUIRE(method.call(12.5, {radix}).string() == "12.5");
        else
            REQUIRE_THROWS_AS(method.call(12.5, {radix}), JSNumberRadixUnsupported);
        REQUIRE(method.call(INFINITY, {radix}).string() == "Infinity");
        REQUIRE(method.call(NAN, {radix}).string() == "NaN");
    }
    REQUIRE_THROWS_AS(method.call(INFINITY, {0}), JSRangeError);
    REQUIRE_THROWS_AS(method.call(NAN, {37}), JSRangeError);
}
TEST_CASE("Number prototypes are heap rooted without owning TLS or boxed cycles", "[number]") {
    isolated([] {
        JSWeakValue weakPrototype, weakMethod, weakPayload, weakBoxed;
        {
            auto realm = JSValue::object(), prototype = nativeNumberPrototype();
            auto payload = JSValue::object();
            own(prototype, "payload", payload);
            weakPrototype = JSWeakValue(prototype);
            weakMethod = JSWeakValue(prototype.get("toFixed"));
            weakPayload = JSWeakValue(payload);
            {
                auto boxed = createNativeNumberObject(3);
                own(boxed, "self", boxed);
                weakBoxed = JSWeakValue(boxed);
            }
            realm.collectGarbage();
            REQUIRE(weakBoxed.expired());
            prototype.erase("toFixed");
            prototype = {};
            payload = {};
            realm.collectGarbage();
            REQUIRE_FALSE(weakPrototype.expired());
            REQUIRE_FALSE(weakMethod.expired());
            REQUIRE_FALSE(weakPayload.expired());
            REQUIRE(nativeNumberPrototype().get("payload").sameValue(*weakPayload.lock()));
        }
        REQUIRE(weakPrototype.expired());
        REQUIRE(weakMethod.expired());
        REQUIRE(weakPayload.expired());
        REQUIRE_FALSE(nativeNumberPrototype().hasOwn("payload"));
    });
}
TEST_CASE("NumberData factories and methods reject foreign thread heap references", "[number]") {
    auto object = createNativeNumberObject(1.25), prototype = nativeNumberPrototype(),
         method = prototype.get("toFixed");
    std::atomic<unsigned> rejected = 0;
    std::thread worker([&] {
        const auto attempt = [&](auto body) {
            try {
                body();
            } catch (const JSTypeError&) {
                ++rejected;
            }
        };
        attempt([&] { isNativeNumberObject(object); });
        attempt([&] { nativeNumberValue(object); });
        attempt([&] { createNativeNumberObject(object); });
        attempt([&] { object.get("toFixed"); });
        attempt([&] { method.call(1.25, {2}); });
        attempt([&] { JSValue(1.25).get("toFixed", object); });
        attempt([&] { nativeNumberPrototype().get("toFixed").call(object, {2}); });
        attempt([&] { nativeNumberPrototype().get("toFixed").call(1.25, {object}); });
    });
    worker.join();
    REQUIRE(rejected == 8);
}

TEST_CASE("Number toFixed retains the selected method across coercion and collection", "[number]") {
    auto realm = JSValue::object(), prototype = nativeNumberPrototype();
    const auto original = *prototype.getOwnPropertyDescriptor("toFixed");
    auto fraction = JSValue::object();
    own(fraction, "valueOf",
        JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&,
                const std::vector<JSValue>& captured) -> JSValue {
                auto prototype = captured[0];
                prototype.erase("toFixed");
                prototype.collectGarbage();
                return 2;
            },
            {prototype}));
    const JSValue number = 1.25;
    const auto result = number.get("toFixed").call(number, {fraction});
    prototype.defineProperty("toFixed", original);
    REQUIRE(result.string() == "1.25");
}

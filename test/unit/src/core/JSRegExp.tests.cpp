#include "core/JSRegExp.h"
#include "catch.hpp"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include <atomic>
#include <exception>
#include <thread>
namespace {
using Args = std::vector<JSValue>;
JSPropertyDescriptor data(JSValue value, bool writable = true) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = writable;
    d.configurable = true;
    return d;
}
JSValue method(const char* name) { return nativeRegExpPrototype().get(name); }
} // namespace
TEST_CASE("RegExp construction preserves slots call identity and new target", "[native-regexp]") {
    auto ctor = nativeRegExpConstructor();
    auto rx = createNativeRegExp("a", "gm");
    REQUIRE(ctor.get("name").string() == "RegExp");
    REQUIRE(ctor.get("length").number() == 2);
    REQUIRE(ctor.call({}, {rx}).sameValue(rx));
    REQUIRE_FALSE(ctor.construct({rx}).sameValue(rx));
    rx.defineProperty("source", data("bad"));
    rx.defineProperty("flags", data("bad"));
    const auto copy = ctor.construct({rx});
    REQUIRE(copy.get("source").string() == "a");
    REQUIRE(copy.get("flags").string() == "gm");
    rx.defineProperty(JSValue::wellKnownSymbol("match"), data(false));
    REQUIRE_FALSE(ctor.call({}, {rx}).sameValue(rx));
    const auto subclass = JSValue::constructor(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    const auto derived = ctor.construct({"b"}, subclass);
    REQUIRE(derived.getPrototype().sameValue(subclass.get("prototype")));
    REQUIRE(isNativeRegExpObject(derived));
    REQUIRE_FALSE(isNativeRegExpObject(nativeRegExpPrototype()));
    REQUIRE_THROWS_AS(method("exec").call(nativeRegExpPrototype(), {""}), JSTypeError);
    REQUIRE(nativeRegExpPrototype().get("source").string() == "(?:)");
    REQUIRE(nativeRegExpPrototype().get("global").isUndefined());
    REQUIRE(JSValue::object().get("toString").call(copy).string() == "[object RegExp]");
    REQUIRE_THROWS_AS(createNativeRegExp("a", "gg"), JSSyntaxError);
    REQUIRE_THROWS_AS(createNativeRegExp("a", "z"), JSSyntaxError);
    REQUIRE_THROWS_AS(createNativeRegExp("a", "uv"), JSSyntaxError);
    for (const auto* flags : {"d", "i", "u", "v"})
        REQUIRE_THROWS_AS(createNativeRegExp("a", flags), JSRegExpUnsupported);
}
TEST_CASE("RegExp lastIndex descriptors preserve arbitrary values and write failures",
          "[native-regexp]") {
    auto rx = createNativeRegExp("a", "g");
    const auto descriptor = rx.getOwnPropertyDescriptor("lastIndex");
    REQUIRE(descriptor->writable == true);
    REQUIRE(descriptor->enumerable == false);
    REQUIRE(descriptor->configurable == false);
    REQUIRE_FALSE(rx.erase("lastIndex"));
    auto token = JSValue::object();
    token.setPrimitiveConverter(
        JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return 1; }));
    rx.set("lastIndex", token);
    REQUIRE(rx.get("lastIndex").sameValue(token));
    REQUIRE(method("exec").call(rx, {"aba"}).get("index").number() == 2);
    REQUIRE(rx.get("lastIndex").number() == 3);
    JSPropertyDescriptor frozen;
    frozen.writable = false;
    rx.defineProperty("lastIndex", frozen);
    REQUIRE_THROWS_AS(method("exec").call(rx, {"aba"}), JSTypeError);
    auto ordinary = createNativeRegExp("a");
    ordinary.set("lastIndex", token);
    ordinary.defineProperty("lastIndex", frozen);
    REQUIRE(method("exec").call(ordinary, {"a"}).get("index").number() == 0);
    REQUIRE(ordinary.get("lastIndex").sameValue(token));
    const auto value = method("exec").call(createNativeRegExp("(a)?b"), {"b"});
    REQUIRE(value.hasOwn("1"));
    REQUIRE(value.get("1").isUndefined());
    REQUIRE(value.hasOwn("groups"));
    REQUIRE(value.getOwnPropertyDescriptor("index")->enumerable == true);
    REQUIRE(value.getOwnPropertyDescriptor("0")->writable == true);
}
TEST_CASE("RegExp methods honor dynamic exec and inherited string hooks", "[native-regexp]") {
    auto rx = createNativeRegExp("a");
    rx.set("exec", nullptr);
    REQUIRE(method("test").call(rx, {"a"}).toBoolean());
    auto generic = JSValue::object();
    generic.set("exec", nullptr);
    REQUIRE_THROWS_AS(method("test").call(generic, {"a"}), JSTypeError);
    rx.set("exec", JSValue::function(
                       +[](const JSValue&, const Args&, const Args&) -> JSValue { return 2; }));
    REQUIRE_THROWS_AS(method("test").call(rx, {"a"}), JSTypeError);
    auto sp = nativeStringPrototype();
    const auto old = sp.getOwnPropertyDescriptor(JSValue::wellKnownSymbol("replace"));
    sp.defineProperty(
        JSValue::wellKnownSymbol("replace"),
        data(JSValue::function(+[](const JSValue& self, const Args& args, const Args&) -> JSValue {
            return self.toString() + ":" + args[0].toString();
        })));
    REQUIRE(JSValue("body").get("replace").call("body", {"needle", "x"}).string() == "needle:body");
    sp.erase(JSValue::wellKnownSymbol("replace"));
    if (old)
        sp.defineProperty(JSValue::wellKnownSymbol("replace"), *old);
    REQUIRE_THROWS_AS(sp.get("replace").call(nullptr, {rx, "x"}), JSTypeError);
    REQUIRE_THROWS_AS(sp.get("split").call(JSValue{}, {rx}), JSTypeError);
}
TEST_CASE("RegExp replacement consumes live results and runs overlapping callbacks",
          "[native-regexp]") {
    auto receiver = JSValue::object();
    receiver.set("flags", "g");
    receiver.set("lastIndex", 0);
    auto state = JSValue::object();
    state.set("i", 0);
    auto result = JSValue::array(1);
    result.set("0", "a");
    result.set("index", 1);
    receiver.set("exec", JSValue::function(
                             +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                                 auto state = c[0];
                                 int i = int(state.get("i").number());
                                 state.set("i", i + 1);
                                 if (i == 2)
                                     return nullptr;
                                 auto result = c[1];
                                 result.set("index", i ? 0 : 1);
                                 return result;
                             },
                             {state, result}));
    state.set("calls", 0);
    auto replacement = JSValue::function(
        +[](const JSValue&, const Args&, const Args& c) -> JSValue {
            auto state = c[0];
            state.set("calls", state.get("calls").number() + 1);
            state.collectGarbage();
            return "X";
        },
        {state});
    const auto output = nativeRegExpPrototype()
                            .get(JSValue::wellKnownSymbol("replace"))
                            .call(receiver, {"abc", replacement});
    REQUIRE(output.string() == "Xbc");
    REQUIRE(state.get("calls").number() == 2);
}
TEST_CASE("String split and regular expressions use UTF16 indices", "[native-regexp]") {
    const JSValue input(jsUTF8(u"a\U0001f600b"));
    const auto parts = input.get("split").call(input, {""});
    REQUIRE(parts.length() == 4);
    REQUIRE(jsUTF16(parts.get("1").string()) == std::u16string(1, 0xd83d));
    REQUIRE(jsUTF16(parts.get("2").string()) == std::u16string(1, 0xde00));
    auto rx = createNativeRegExp(".", "g");
    const auto exec = method("exec");
    REQUIRE(exec.call(rx, {input}).get("index").number() == 0);
    REQUIRE(exec.call(rx, {input}).get("index").number() == 1);
    REQUIRE(exec.call(rx, {input}).get("index").number() == 2);
    REQUIRE(JSValue(jsUTF8(u"\u1680\ufeffx\u2028"))
                .get("trim")
                .call(jsUTF8(u"\u1680\ufeffx\u2028"))
                .string() == "x");
    REQUIRE(JSValue("Straße ﬃ").get("toUpperCase").call("Straße ﬃ").string() == "STRASSE FFI");
}
TEST_CASE("Supported String and RegExp methods have intrinsic descriptors", "[native-regexp]") {
    const auto proto = nativeRegExpPrototype();
    for (const auto* name : {"exec", "test", "toString"}) {
        const auto fn = proto.get(name);
        REQUIRE_FALSE(fn.isConstructible());
        REQUIRE_FALSE(fn.hasOwn("prototype"));
        REQUIRE(proto.getOwnPropertyDescriptor(name)->enumerable == false);
        REQUIRE(fn.get("name").string() == name);
    }
    for (const auto* name : {"replace", "split", "match"}) {
        const auto key = JSValue::wellKnownSymbol(name);
        const auto fn = proto.get(key);
        REQUIRE_FALSE(fn.isConstructible());
        REQUIRE(fn.get("name").string() == std::string("[Symbol.") + name + "]");
        REQUIRE(proto.getOwnPropertyDescriptor(key)->configurable == true);
    }
    for (const auto* name : {"source", "flags", "global", "sticky", "unicodeSets"}) {
        const auto d = proto.getOwnPropertyDescriptor(name);
        REQUIRE(d->setter->isUndefined());
        REQUIRE(d->getter->get("name").string() == std::string("get ") + name);
        REQUIRE(d->getter->get("length").number() == 0);
        REQUIRE_FALSE(d->getter->isConstructible());
    }
    for (const auto* name : {"replace", "split", "trim", "toUpperCase"}) {
        const auto fn = JSValue("").get(name);
        REQUIRE_FALSE(fn.isConstructible());
        REQUIRE(fn.get("name").string() == name);
        REQUIRE(nativeStringPrototype().getOwnPropertyDescriptor(name)->enumerable == false);
    }
}
TEST_CASE("RegExp managed edges and fresh heaps release without native cycles", "[native-regexp]") {
    JSWeakValue constructorWeak, methodWeak, payloadWeak;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            const auto realm = JSValue::object();
            auto ctor = nativeRegExpConstructor();
            constructorWeak = JSWeakValue(ctor);
            auto fn = nativeRegExpPrototype().get("exec");
            methodWeak = JSWeakValue(fn);
            auto payload = JSValue::object();
            payloadWeak = JSWeakValue(payload);
            fn.set("payload", payload);
            nativeRegExpPrototype().erase("exec");
            payload = {};
            fn = {};
            realm.collectGarbage();
            REQUIRE_FALSE(payloadWeak.expired());
            JSWeakValue instanceWeak, cycleWeak;
            {
                auto instance = createNativeRegExp("a");
                auto cycle = JSValue::object();
                cycle.set("instance", instance);
                instance.set("lastIndex", cycle);
                instanceWeak = JSWeakValue(instance);
                cycleWeak = JSWeakValue(cycle);
            }
            realm.collectGarbage();
            REQUIRE(instanceWeak.expired());
            REQUIRE(cycleWeak.expired());
        } catch (...) {
            error = std::current_exception();
        }
    });
    worker.join();
    if (error)
        std::rethrow_exception(error);
    // Cross-thread weak inspection is deliberately forbidden; inspect expiry on
    // the owner thread before its exit in a second scoped-lifetime fixture.
    std::thread lifecycle([&] {
        try {
            JSWeakValue a, b;
            {
                auto ctor = nativeRegExpConstructor();
                a = JSWeakValue(ctor);
                b = JSWeakValue(nativeRegExpPrototype().get("exec"));
            }
            REQUIRE(a.expired());
            REQUIRE(b.expired());
        } catch (...) {
            error = std::current_exception();
        }
    });
    lifecycle.join();
    if (error)
        std::rethrow_exception(error);
}
TEST_CASE("RegExp foreign thread access fails before callbacks", "[native-regexp]") {
    const auto receiver = createNativeRegExp("a");
    const auto fn = method("exec");
    std::atomic<unsigned> failures = 0;
    std::thread worker([&] {
        auto check = [&](auto operation) {
            try {
                operation();
            } catch (const std::logic_error&) {
                ++failures;
            }
        };
        check([&] { isNativeRegExpObject(receiver); });
        check([&] { createNativeRegExp(receiver); });
        check([&] { fn.call(receiver, {"a"}); });
        check([&] { receiver.get("source"); });
    });
    worker.join();
    REQUIRE(failures == 4);
}

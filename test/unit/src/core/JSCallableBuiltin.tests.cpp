#include "catch.hpp"
#include "core/JSValue.h"
#include <exception>
#include <thread>
namespace {
using Args = std::vector<JSValue>;
JSValue builtin(const JSValue& receiver, const JSValue& target, const Args& args,
                const Args& captures) {
    auto trace = captures[0];
    trace.set("receiver", receiver);
    trace.set("target", target);
    trace.set("argument", args.empty() ? JSValue{} : args[0]);
    if (target.isUndefined())
        return args.empty() ? JSValue{} : args[0];
    auto result = JSValue::object();
    result.set("argument", args.empty() ? JSValue{} : args[0]);
    return result;
}
} // namespace
TEST_CASE("Callable builtins receive explicit call and construction contexts",
          "[callable-builtin]") {
    auto trace = JSValue::object();
    auto constructor = JSValue::callableBuiltinConstructor(builtin, {trace}, "Audit", 1);
    const auto receiver = JSValue::object();
    const auto argument = JSValue::object();
    REQUIRE(constructor.isCallable());
    REQUIRE(constructor.isConstructible());
    REQUIRE(constructor.get("name").string() == "Audit");
    REQUIRE(constructor.get("length").number() == 1);
    REQUIRE(constructor.call(receiver, {argument}).sameValue(argument));
    REQUIRE(trace.get("receiver").sameValue(receiver));
    REQUIRE(trace.get("target").isUndefined());
    const auto instance = constructor.construct({argument});
    REQUIRE(instance.get("argument").sameValue(argument));
    REQUIRE(trace.get("receiver").isUndefined());
    REQUIRE(trace.get("target").sameValue(constructor));
    const auto subclass = JSValue::constructor(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    REQUIRE(constructor.construct({7}, subclass).get("argument").number() == 7);
    REQUIRE(trace.get("target").sameValue(subclass));
    REQUIRE(constructor.call(instance, {3}).number() == 3);
    REQUIRE(trace.get("receiver").sameValue(instance));
    REQUIRE(trace.get("target").isUndefined());
}
TEST_CASE("Callable builtin allocation and exceptional results remain explicit",
          "[callable-builtin]") {
    const auto primitive = JSValue::callableBuiltinConstructor(
        +[](const JSValue&, const JSValue&, const Args&, const Args&) -> JSValue { return 7; });
    REQUIRE(primitive.call().number() == 7);
    REQUIRE_THROWS_AS(primitive.construct(), JSTypeError);
    const auto throwing = JSValue::callableBuiltinConstructor(
        +[](const JSValue&, const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSRangeError("sentinel");
        });
    REQUIRE_THROWS_AS(throwing.call(), JSRangeError);
    REQUIRE_THROWS_AS(throwing.construct(), JSRangeError);
    const auto fn =
        JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    REQUIRE_THROWS_AS(primitive.construct({}, fn), JSTypeError);
    const auto old = JSValue::builtinConstructor(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return JSValue::object(); });
    REQUIRE_THROWS_AS(old.call(), JSTypeError);
    REQUIRE(old.construct().isObject());
    REQUIRE_THROWS_AS(JSValue::callableBuiltinConstructor(nullptr), JSTypeError);
}
TEST_CASE("Callable builtin captures stay live during reentry and collect cycles",
          "[callable-builtin]") {
    auto realm = JSValue::object();
    JSWeakValue weak;
    {
        auto payload = JSValue::object();
        auto constructor = JSValue::callableBuiltinConstructor(
            +[](const JSValue&, const JSValue& target, const Args& args,
                const Args& captures) -> JSValue {
                captures[0].collectGarbage();
                if (target.isUndefined())
                    return captures[0];
                const auto again = captures[0].get("constructor").call({}, args);
                auto result = JSValue::object();
                result.set("payload", again);
                return result;
            },
            {payload});
        payload.set("constructor", constructor);
        weak = JSWeakValue(payload);
        REQUIRE(constructor.construct().get("payload").sameValue(payload));
    }
    realm.collectGarbage();
    REQUIRE(weak.expired());
}

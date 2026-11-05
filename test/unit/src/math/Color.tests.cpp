/**
 * @file   Color.tests.cpp
 * @brief  Color 单元测试
 * @author Abnerring
 * @date   2025-2026
 */

#include "catch.hpp"
#include "math/Color.h"
#include <cmath>
#include <type_traits>

TEST_CASE("Color components are native properties in the working color space", "[properties]") {
    static_assert(std::is_same_v<decltype(Color().r), double>);
    Color color;
    color.r = .2; color.g = .4; color.b = .6;
    REQUIRE(color.toArray() == std::vector<double>{.2, .4, .6});
    Color copied = color;
    copied.r *= 2;
    REQUIRE(color.r == .2);
    REQUIRE(copied.r == .4);
    color.setRGB(.1, .2, .3);
    REQUIRE(color.r == .1);
    REQUIRE(color.g == .2);
    REQUIRE(color.b == .3);
}
#include "math/Vector3.h"
#include "math/Vector4.h"

#define EPSILON 1e-6

TEST_CASE("Color Construction")
{
    SECTION("Default constructor - white")
    {
        Color c;
        REQUIRE(c.r == 1.0);
        REQUIRE(c.g == 1.0);
        REQUIRE(c.b == 1.0);
    }

    SECTION("RGB constructor")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.r == 1.0);
        REQUIRE(c.g == 0.0);
        REQUIRE(c.b == 0.0);
    }

    SECTION("Hex constructor")
    {
        Color c(0xff0000);
        REQUIRE(c.r == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.g == Approx(0.0).epsilon(EPSILON));
        REQUIRE(c.b == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color setters")
{
    SECTION("set()")
    {
        Color c;
        c.set(0.5, 0.5, 0.5);
        REQUIRE(c.r == 0.5);
        REQUIRE(c.g == 0.5);
        REQUIRE(c.b == 0.5);
    }

    SECTION("setScalar()")
    {
        Color c;
        c.setScalar(0.5);
        REQUIRE(c.r == 0.5);
        REQUIRE(c.g == 0.5);
        REQUIRE(c.b == 0.5);
    }

    SECTION("setHex()")
    {
        Color c;
        c.setHex(0x00ff00);
        REQUIRE(c.r == Approx(0.0).epsilon(EPSILON));
        REQUIRE(c.g == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.b == Approx(0.0).epsilon(EPSILON));
    }

    SECTION("setHSL()")
    {
        Color c;
        c.setHSL(0.0, 1.0, 0.5);  // Red
        REQUIRE(c.r == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color copy")
{
    SECTION("copy()")
    {
        Color a(0.1, 0.2, 0.3);
        Color b;
        b.copy(a);
        REQUIRE(b.r == Approx(0.1).epsilon(EPSILON));
        REQUIRE(b.g == Approx(0.2).epsilon(EPSILON));
        REQUIRE(b.b == Approx(0.3).epsilon(EPSILON));
    }

    SECTION("clone()")
    {
        Color a(0.3, 0.4, 0.5);
        Color b = a.clone();
        REQUIRE(b.r == Approx(0.3).epsilon(EPSILON));
    }
}

TEST_CASE("Color arithmetic operations")
{
    SECTION("add()")
    {
        Color c(0.1, 0.2, 0.3);
        Color other(0.1, 0.1, 0.1);
        c.add(other);
        REQUIRE(c.r == Approx(0.2).epsilon(EPSILON));
        REQUIRE(c.g == Approx(0.3).epsilon(EPSILON));
        REQUIRE(c.b == Approx(0.4).epsilon(EPSILON));
    }

    SECTION("sub()")
    {
        Color c(0.5, 0.5, 0.5);
        Color other(0.1, 0.1, 0.1);
        c.sub(other);
        REQUIRE(c.r == Approx(0.4).epsilon(EPSILON));
    }

    SECTION("multiplyScalar()")
    {
        Color c(0.5, 0.5, 0.5);
        c.multiplyScalar(2.0);
        REQUIRE(c.r == Approx(1.0).epsilon(EPSILON));
    }

    SECTION("divideScalar()")
    {
        Color c(1.0, 1.0, 1.0);
        c.divideScalar(2.0);
        REQUIRE(c.r == Approx(0.5).epsilon(EPSILON));
    }
}

TEST_CASE("Color lerp")
{
    SECTION("lerp()")
    {
        Color a(0.0, 0.0, 0.0);
        Color b(1.0, 1.0, 1.0);
        a.lerp(b, 0.5);
        REQUIRE(a.r == Approx(0.5).epsilon(EPSILON));
        REQUIRE(a.g == Approx(0.5).epsilon(EPSILON));
        REQUIRE(a.b == Approx(0.5).epsilon(EPSILON));
    }

    SECTION("lerp at boundaries")
    {
        Color a(0.0, 0.0, 0.0);
        Color b(1.0, 1.0, 1.0);
        Color c = a;
        c.lerp(b, 0.0);
        REQUIRE(c.r == Approx(0.0).epsilon(EPSILON));
        c.lerp(b, 1.0);
        REQUIRE(c.r == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color equals")
{
    SECTION("equals()")
    {
        Color a(0.1, 0.2, 0.3);
        Color b(0.1, 0.2, 0.3);
        Color c(0.1, 0.2, 0.4);
        REQUIRE(a.equals(b));
        REQUIRE_FALSE(a.equals(c));
    }
}

TEST_CASE("Color getHex")
{
    SECTION("getHex()")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.getHex() == 0xff0000);
    }
}

TEST_CASE("Color getHexString")
{
    SECTION("getHexString()")
    {
        Color c(1.0, 0.0, 0.0);
        REQUIRE(c.getHexString() == "ff0000");
    }
}

TEST_CASE("Color getHSL")
{
    SECTION("getHSL()")
    {
        Color c(1.0, 0.0, 0.0);  // Red
        Vector3 hsl;
        c.getHSL(hsl);
        REQUIRE(hsl.x == Approx(0.0).epsilon(EPSILON));  // Hue
        REQUIRE(hsl.y == Approx(1.0).epsilon(EPSILON));  // Saturation
        REQUIRE(hsl.z == Approx(0.5).epsilon(EPSILON));  // Lightness
    }
}

TEST_CASE("Color offsetHSL")
{
    SECTION("offsetHSL()")
    {
        Color c(1.0, 0.0, 0.0);  // Red
        c.offsetHSL(0.0, 0.0, 0.2);  // Increase lightness
        Vector3 hsl;
        c.getHSL(hsl);
        REQUIRE(hsl.z == Approx(0.7).epsilon(EPSILON));
    }
}

TEST_CASE("Color fromVector3")
{
    SECTION("fromVector3()")
    {
        Color c;
        Vector3 v(1.0, 0.5, 0.0);
        c.fromVector3(v);
        REQUIRE(c.r == Approx(1.0).epsilon(EPSILON));
        REQUIRE(c.g == Approx(0.5).epsilon(EPSILON));
        REQUIRE(c.b == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color toVector3")
{
    SECTION("toVector3()")
    {
        Color c(1.0, 0.5, 0.0);
        Vector3 v;
        c.toVector3(v);
        REQUIRE(v.x == Approx(1.0).epsilon(EPSILON));
        REQUIRE(v.y == Approx(0.5).epsilon(EPSILON));
        REQUIRE(v.z == Approx(0.0).epsilon(EPSILON));
    }
}

TEST_CASE("Color toVector4")
{
    SECTION("toVector4()")
    {
        Color c(1.0, 0.5, 0.0);
        Vector4 v;
        c.toVector4(v);
        REQUIRE(v.x == Approx(1.0).epsilon(EPSILON));
        REQUIRE(v.y == Approx(0.5).epsilon(EPSILON));
        REQUIRE(v.z == Approx(0.0).epsilon(EPSILON));
        REQUIRE(v.w == Approx(1.0).epsilon(EPSILON));
    }
}

TEST_CASE("Managed Color multiplyScalar updates RGB and returns the receiver", "[managed-color]") {
    const auto ctor = nativeColorConstructor();
    const auto prototype = ctor.get("prototype");
    const auto fn = prototype.get("multiplyScalar");
    const auto descriptor = prototype.getOwnPropertyDescriptor("multiplyScalar");
    REQUIRE(fn.isCallable());
    REQUIRE(fn.get("length").strictEquals(1));
    REQUIRE(fn.get("name").strictEquals("multiplyScalar"));
    REQUIRE_FALSE(fn.isConstructible());
    REQUIRE(descriptor->writable.value_or(false));
    REQUIRE(descriptor->configurable.value_or(false));
    REQUIRE_FALSE(descriptor->enumerable.value_or(false));
    const auto color = ctor.construct({0.25, 0.5, 0.75});
    REQUIRE(fn.call(color, {2}).sameValue(color));
    REQUIRE(color.get("r").strictEquals(0.5));
    REQUIRE(color.get("g").strictEquals(1));
    REQUIRE(color.get("b").strictEquals(1.5));
    fn.call(color, {-0.0});
    REQUIRE(std::signbit(color.get("r").number()));
    REQUIRE(std::signbit(color.get("g").number()));
    REQUIRE(std::signbit(color.get("b").number()));
    fn.call(color);
    REQUIRE(std::isnan(color.get("r").number()));
    REQUIRE(std::isnan(color.get("g").number()));
    REQUIRE(std::isnan(color.get("b").number()));
}

TEST_CASE("Managed Color multiplyScalar observes channel access and repeated scalar conversion", "[managed-color]") {
    using Args = std::vector<JSValue>;
    auto events = JSValue::array();
    auto fields = JSValue::object();
    fields.set("r", 1); fields.set("g", 2); fields.set("b", 3);
    auto state = JSValue::object();
    state.set("fields", fields); state.set("events", events); state.set("calls", 0);
    auto receiver = JSValue::object();
    for (const auto* key : {"r", "g", "b"}) {
        JSPropertyDescriptor descriptor;
        descriptor.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            const auto log = c[0].get("events");
            log.get("push").call(log, {"get:" + c[1].string()});
            return c[0].get("fields").get(c[1]);
        }, {state, key});
        descriptor.setter = JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
            const auto log = c[0].get("events");
            log.get("push").call(log, {"set:" + c[1].string()});
            auto fields = c[0].get("fields");
            fields.set(c[1], args[0]);
            return {};
        }, {state, key});
        receiver.defineProperty(key, descriptor);
    }
    auto scalar = JSValue::object();
    scalar.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args& args, const Args& c) -> JSValue {
        auto state = c[0];
        const auto calls = state.get("calls").number() + 1;
        state.set("calls", calls);
        const auto log = state.get("events");
        log.get("push").call(log, {"scalar:" + args[0].string()});
        auto fields = state.get("fields");
        if (calls == 1) fields.set("g", 10);
        if (calls == 2) fields.set("b", 20);
        return calls + 1;
    }, {state}));
    const auto fn = nativeColorConstructor().get("prototype").get("multiplyScalar");
    REQUIRE(fn.call(receiver, {scalar}).sameValue(receiver));
    REQUIRE(fields.get("r").strictEquals(2));
    REQUIRE(fields.get("g").strictEquals(30));
    REQUIRE(fields.get("b").strictEquals(80));
    REQUIRE(state.get("calls").strictEquals(3));
    const std::vector<std::string> expected{
        "get:r", "scalar:number", "set:r", "get:g", "scalar:number", "set:g", "get:b", "scalar:number", "set:b"};
    REQUIRE(events.length() == expected.size());
    for (unsigned i = 0; i < expected.size(); ++i) REQUIRE(events.get(JSValue(i)).strictEquals(expected[i]));
}

TEST_CASE("Managed Color multiplyScalar preserves earlier writes on abrupt completion", "[managed-color]") {
    using Args = std::vector<JSValue>;
    const auto fn = nativeColorConstructor().get("prototype").get("multiplyScalar");
    auto receiver = JSValue::object();
    receiver.set("r", 1); receiver.set("g", 2); receiver.set("b", 3);
    SECTION("A nonwritable channel stops subsequent channels") {
        JSPropertyDescriptor descriptor; descriptor.writable = false;
        receiver.defineProperty("g", descriptor);
        REQUIRE_THROWS_AS(fn.call(receiver, {3}), JSTypeError);
    }
    SECTION("Scalar coercion is repeated and may throw before the second write") {
        auto state = JSValue::object(); state.set("calls", 0);
        auto scalar = JSValue::object();
        scalar.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
            auto state = c[0];
            const auto calls = state.get("calls").number() + 1;
            state.set("calls", calls);
            if (calls == 2) throw JSTypeError("scalar conversion");
            return 3;
        }, {state}));
        REQUIRE_THROWS_AS(fn.call(receiver, {scalar}), JSTypeError);
        REQUIRE(state.get("calls").strictEquals(2));
    }
    REQUIRE(receiver.get("r").strictEquals(3));
    REQUIRE(receiver.get("g").strictEquals(2));
    REQUIRE(receiver.get("b").strictEquals(3));
}

#include "catch.hpp"
#include "parity/NativeMaterialsMathFixture.h"
using namespace MaterialsMathFixture;
TEST_CASE("Materials matrix protocols update native storage and preserve alias identities",
          "[core][native][materials]") {
    auto owner = std::make_shared<Texture>();
    owner->offset.set(.2, -.3);
    owner->repeat.set(2, 3);
    owner->center.set(.4, .6);
    owner->rotation = .7;
    auto t = nativeTexture(owner), m = t.get("matrix"), elements = m.get("elements");
    REQUIRE(invoke(t, "updateMatrix").isUndefined());
    REQUIRE(m.sameValue(t.get("matrix")));
    REQUIRE(elements.sameValue(m.get("elements")));
    REQUIRE(owner->matrix.elements()[0] == 2 * JSNumberMath::cos(.7));
    elements.set("6", 7);
    REQUIRE(owner->matrix.elements()[6] == 7);
    owner->matrix.elements()[7] = 9;
    REQUIRE(elements.get("7").strictEquals(9));
    t.set("colorSpace", SRGBColorSpace);
    REQUIRE(owner->colorSpace == SRGBColorSpace);
    owner->colorSpace = LinearSRGBColorSpace;
    REQUIRE(t.get("colorSpace").strictEquals(LinearSRGBColorSpace));
    auto copy = nativeMatrix(std::make_shared<Matrix3>());
    REQUIRE(invoke(copy, "copy", {m}).sameValue(copy));
    REQUIRE(copy.get("elements").get("6").strictEquals(7));
    REQUIRE(copy.getPrototype().sameValue(nativeMatrix3Prototype()));
    REQUIRE(nativeMatrix(std::make_shared<Matrix4>())
                .getPrototype()
                .sameValue(nativeMatrix4Prototype()));
}
TEST_CASE("Materials protocols remain ordinary class methods with exact metadata",
          "[core][native][materials]") {
    const std::vector<std::tuple<JSValue, const char*, unsigned>> methods = {
        {matrix(), "copy", 1},           {matrix(), "multiply", 1},
        {matrix(), "premultiply", 1},    {matrix(), "multiplyMatrices", 2},
        {matrix(), "setUvTransform", 7}, {matrix({}, true), "makeRotationFromEuler", 1},
        {vector(), "negate", 0},         {color(), "getRGB", 1},
        {texture(), "updateMatrix", 0}};
    for (const auto& [receiver, name, arity] : methods) {
        const auto d = receiver.getPrototype().getOwnPropertyDescriptor(name);
        REQUIRE(d);
        REQUIRE(d->writable.value_or(false));
        REQUIRE(d->configurable.value_or(false));
        REQUIRE_FALSE(d->enumerable.value_or(true));
        auto fn = receiver.get(name);
        REQUIRE(fn.get("name").strictEquals(name));
        REQUIRE(fn.get("length").strictEquals(arity));
        REQUIRE_FALSE(fn.isConstructible());
    }
}
TEST_CASE("Materials chaining respects replacement returns and discarded texture return",
          "[core][native][materials]") {
    REQUIRE(chaining(0).at("result").at("value").get<std::string>() == "replacement");
    REQUIRE(chaining(1).at("result").at("value").get<std::string>() == "replacement");
    REQUIRE(chaining(2).at("result").at("value").get<std::string>() == "self");
    REQUIRE(chaining(3).at("result").at("value").get<std::string>() == "@undefined");
    const auto nested = reentry(0);
    REQUIRE(nested.at("r").get<double>() == .1);
    REQUIRE(nested.at("g").get<double>() == .7);
    REQUIRE(nested.at("b").get<double>() == .6);
    const auto overridden = colorProtocol(0);
    REQUIRE(overridden.at("target")[0].get<double>() == .1);
    REQUIRE(overridden.at("other")[0].get<double>() != .4);
}
TEST_CASE("Native texture owner retains matrix extensions without managed handles",
          "[core][native][materials][gc]") {
    auto anchor = JSValue::object();
    auto owner = std::make_shared<Texture>();
    JSWeakValue tWeak, mWeak, eWeak, extensionWeak;
    std::weak_ptr<Texture> nativeWeak = owner;
    {
        auto t = nativeTexture(owner), m = t.get("matrix"), elements = m.get("elements"),
             extension = object({{"texture", t}});
        data(m, "extension", extension);
        data(elements, "extension", extension);
        tWeak = JSWeakValue(t);
        mWeak = JSWeakValue(m);
        eWeak = JSWeakValue(elements);
        extensionWeak = JSWeakValue(extension);
    }
    anchor.collectGarbage();
    REQUIRE_FALSE(tWeak.expired());
    REQUIRE_FALSE(mWeak.expired());
    REQUIRE_FALSE(eWeak.expired());
    REQUIRE_FALSE(extensionWeak.expired());
    {
        auto t = nativeTexture(owner), m = t.get("matrix");
        REQUIRE(m.sameValue(*mWeak.lock()));
        REQUIRE(m.get("extension").sameValue(*extensionWeak.lock()));
        REQUIRE(m.get("elements").sameValue(*eWeak.lock()));
    }
    owner.reset();
    anchor.collectGarbage();
    REQUIRE(tWeak.expired());
    REQUIRE(mWeak.expired());
    REQUIRE(eWeak.expired());
    REQUIRE(extensionWeak.expired());
    REQUIRE(nativeWeak.expired());
}
TEST_CASE("Texture matrix handle alone owns native texture and collects owner cycle",
          "[core][native][materials][gc]") {
    auto anchor = JSValue::object();
    JSValue held;
    JSWeakValue tWeak, mWeak, eWeak;
    std::weak_ptr<Texture> nativeWeak;
    {
        auto owner = std::make_shared<Texture>();
        nativeWeak = owner;
        auto t = nativeTexture(owner), m = t.get("matrix");
        held = m.get("elements");
        tWeak = JSWeakValue(t);
        mWeak = JSWeakValue(m);
        eWeak = JSWeakValue(held);
        data(held, "owner", t);
    }
    anchor.collectGarbage();
    REQUIRE_FALSE(nativeWeak.expired());
    REQUIRE_FALSE(tWeak.expired());
    REQUIRE_FALSE(mWeak.expired());
    REQUIRE_FALSE(eWeak.expired());
    held.set("0", 4);
    REQUIRE(nativeWeak.lock()->matrix.elements()[0] == 4);
    held = {};
    anchor.collectGarbage();
    REQUIRE(nativeWeak.expired());
    REQUIRE(tWeak.expired());
    REQUIRE(mWeak.expired());
    REQUIRE(eWeak.expired());
}
TEST_CASE("Public matrix prototype access constructs no observable matrix",
          "[core][native][materials]") {
    for (auto prototype : {nativeMatrix3Prototype(), nativeMatrix4Prototype()}) {
        auto state = object({{"calls", 0}});
        JSPropertyDescriptor d;
        d.configurable = true;
        d.setter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                data(c[0], "calls", c[0].get("calls").number() + 1);
                return {};
            },
            {state});
        const auto old = prototype.getOwnPropertyDescriptor("elements");
        prototype.defineProperty("elements", d);
        const auto p3 = nativeMatrix3Prototype(), p4 = nativeMatrix4Prototype();
        const bool identity = p3.sameValue(prototype) || p4.sameValue(prototype);
        const auto count = state.get("calls").number();
        if (old)
            prototype.defineProperty("elements", *old);
        else
            prototype.erase("elements");
        REQUIRE(identity);
        REQUIRE(count == 0);
    }
}



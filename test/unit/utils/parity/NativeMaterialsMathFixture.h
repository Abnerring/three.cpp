#pragma once
#include "math/Color.h"
#include "math/ColorManagement.h"
#include "math/JSNumberMath.h"
#include "math/Vector2.h"
#include "parity/NativeClippingMathFixture.h"
#include "textures/Texture.h"
namespace MaterialsMathFixture {
using ClippingMathFixture::A;
using ClippingMathFixture::accessor;
using ClippingMathFixture::Args;
using ClippingMathFixture::array;
using ClippingMathFixture::attempt;
using ClippingMathFixture::callback;
using ClippingMathFixture::data;
using ClippingMathFixture::encode;
using ClippingMathFixture::invoke;
using ClippingMathFixture::O;
using ClippingMathFixture::observedNumber;
using ClippingMathFixture::trace;
inline JSValue object(std::initializer_list<std::pair<const char*, JSValue>> fields = {}) {
    auto out = JSValue::object();
    for (const auto& [key, value] : fields)
        data(out, key, value);
    return out;
}
inline JSValue matrix(const Args& values = {}, bool four = false) {
    auto out = JSValue::object(four ? nativeMatrix4Prototype() : nativeMatrix3Prototype());
    data(out, "elements", array(values));
    return out;
}
inline JSValue vector(const Args& values = {0, 0}) {
    auto out = JSValue::object(nativeVector(std::make_shared<Vector2>()).getPrototype());
    data(out, "x", values[0]);
    data(out, "y", values[1]);
    return out;
}
inline JSValue color(const Args& values = {1, 1, 1}) {
    auto out = JSValue::object(nativeColor(std::make_shared<Color>()).getPrototype());
    data(out, "isColor", true);
    for (unsigned i = 0; i < 3; ++i)
        data(out, std::string(1, "rgb"[i]), values[i]);
    return out;
}
inline JSValue texture() {
    auto out = JSValue::object(nativeTexture(std::make_shared<Texture>()).getPrototype());
    data(out, "matrix", matrix());
    data(out, "offset", vector({.2, -.1}));
    data(out, "repeat", vector({2, 3}));
    data(out, "center", vector({.5, .5}));
    data(out, "rotation", .7);
    return out;
}
inline JSValue values(JSValue target, const char* fields) {
    Args out;
    for (const char* c = fields; *c; ++c)
        out.push_back(target.get(std::string(1, *c)));
    return array(out);
}
inline JsonValue numeric(unsigned seed, bool native) {
    const char* orders[] = {"XYZ", "YXZ", "ZXY", "ZYX", "YZX", "XZY", "invalid"};
    const double angle = seed == 0   ? -0.
                         : seed == 1 ? 1e30
                         : seed == 2 ? std::numeric_limits<double>::infinity()
                         : seed == 3 ? std::nan("")
                                     : (double(seed) - 32) / 7.;
    auto m = native ? nativeMatrix(std::make_shared<Matrix4>()) : matrix({}, true);
    for (unsigned i = 0; i < 16; ++i)
        data(m.get("elements"), std::to_string(i), double(i) + .125);
    auto euler =
        object({{"x", angle}, {"y", angle * .3}, {"z", -angle * .7}, {"order", orders[seed % 7]}});
    auto r = invoke(m, "makeRotationFromEuler", {euler});
    O out{{"kind", "numeric"},
          {"seed", seed},
          {"native", native},
          {"rotation", encode(m.get("elements"))},
          {"rotationSelf", r.sameValue(m)}};
    auto a = native ? nativeMatrix(std::make_shared<Matrix3>()) : matrix(), b = matrix();
    for (unsigned i = 0; i < 9; ++i) {
        data(a.get("elements"), std::to_string(i), double(i) - 3);
        data(b.get("elements"), std::to_string(i), double((seed * 17 + i * 11) % 19) - 7);
    }
    r = invoke(a, "copy", {b});
    out["copy"] = encode(a.get("elements"));
    out["copySelf"] = r.sameValue(a);
    r = invoke(a, "premultiply", {a});
    out["square"] = encode(a.get("elements"));
    out["squareSelf"] = r.sameValue(a);
    r = invoke(a, "multiply", {b});
    out["multiply"] = encode(a.get("elements"));
    out["multiplySelf"] = r.sameValue(a);
    r = invoke(a, "setUvTransform", {.2, -.4, 2, -3, angle, .5, -.25});
    out["uv"] = encode(a.get("elements"));
    out["uvSelf"] = r.sameValue(a);
    auto v = native ? nativeVector(std::make_shared<Vector2>(-0., angle)) : vector({-0., angle});
    out["negateSelf"] = invoke(v, "negate").sameValue(v);
    out["vector"] = encode(values(v, "xy"));
    return out;
}
inline JsonValue colorNumeric(unsigned seed, unsigned mode) {
    const auto previous = ColorManagement::workingColorSpace;
    const auto enabled = ColorManagement::enabled;
    ColorManagement::workingColorSpace = mode % 2 ? SRGBColorSpace : LinearSRGBColorSpace;
    ColorManagement::enabled = mode < 4;
    Args rgb;
    for (unsigned i = 0; i < 3; ++i)
        rgb.push_back(seed == 0   ? JSValue(-0.)
                      : seed == 1 ? JSValue(std::nan(""))
                      : seed == 2 ? JSValue(std::numeric_limits<double>::infinity())
                                  : JSValue((double((seed * 31 + i * 13) % 101) - 20) / 83.));
    auto source = color(rgb), target = color();
    const JSValue space = mode < 2    ? JSValue(mode == 0 ? SRGBColorSpace : LinearSRGBColorSpace)
                          : mode < 4  ? JSValue()
                          : mode == 4 ? JSValue("bogus")
                          : mode == 5 ? JSValue(nullptr)
                                      : JSValue(false);
    auto result = attempt([&] {
        return invoke(source, "getRGB", {target, space}).sameValue(target) ? JSValue("target")
                                                                           : JSValue();
    });
    ColorManagement::workingColorSpace = previous;
    ColorManagement::enabled = enabled;
    return O{{"kind", "color"},
             {"seed", seed},
             {"mode", mode},
             {"result", result},
             {"rgb", encode(values(target, "rgb"))},
             {"source", encode(values(source, "rgb"))}};
}
inline JsonValue observed(unsigned kind, unsigned mode) {
    const char* names[] = {"makeRotationFromEuler", "copy",   "multiplyMatrices", "premultiply",
                           "setUvTransform",        "negate", "getRGB",           "updateMatrix"};
    const auto* name = names[kind];
    auto state = object({{"log", array()}, {"mode", mode}}), stores = array();
    auto create = [&](const char* label, const char* fields, bool indices, unsigned count) {
        auto store = object(), target = indices ? array() : object();
        for (unsigned i = 0; i < count; ++i) {
            const auto key = indices ? std::to_string(i) : std::string(1, fields[i]);
            double n = (double(i) + 1) / 8;
            JSValue value =
                mode == 1 ? observedNumber(state, std::string(label) + ":" + key, n) : JSValue(n);
            if (mode == 4 && (key == "x" || key == "r"))
                value = "2";
            data(store, key, value);
            accessor(target, key, store, state, label);
        }
        data(stores, std::to_string(stores.length()), store);
        return target;
    };
    auto ma = matrix(), mb = matrix(), mc = matrix({}, true);
    auto ea = create("a", "", true, 9), eb = create("b", "", true, 9),
         ec = create("c", "", true, 16);
    for (const auto& [target, els, label] :
         {std::tuple{ma, ea, "ma"}, std::tuple{mb, eb, "mb"}, std::tuple{mc, ec, "mc"}}) {
        auto store = object({{"elements", els}});
        accessor(target, "elements", store, state, label);
    }
    auto xy = create("xy", "xy", false, 2);
    xy.setPrototype(vector().getPrototype());
    auto rgb = create("rgb", "rgb", false, 3);
    rgb.setPrototype(color().getPrototype());
    auto targetRGB = create("target", "rgb", false, 3);
    targetRGB.setPrototype(color().getPrototype());
    auto euler = create("euler", "xyz", false, 3);
    auto eStore = object({{"order", mode == 4 ? JSValue("invalid") : JSValue("YZX")}});
    accessor(euler, "order", eStore, state, "euler");
    JSValue receiver = ma;
    Args args;
    if (kind == 0) {
        receiver = mc;
        args = {euler};
    }
    if (kind == 1)
        args = {mb};
    if (kind == 2)
        args = {ma, mb};
    if (kind == 3)
        args = {mb};
    if (kind == 4) {
        for (unsigned i = 0; i < 7; ++i)
            args.push_back(mode == 1
                               ? observedNumber(state, "arg" + std::to_string(i), double(i) / 8)
                           : mode == 4 && i < 2 ? JSValue(std::to_string(i + 1))
                                                : JSValue(double(i) / 8));
    }
    if (kind == 5)
        receiver = xy;
    if (kind == 6) {
        receiver = rgb;
        args = {targetRGB, mode == 4 ? JSValue("bogus") : JSValue(SRGBColorSpace)};
    }
    if (kind == 7) {
        receiver = texture();
        auto store =
            object({{"matrix", ma},
                    {"offset", xy},
                    {"repeat", xy},
                    {"center", xy},
                    {"rotation", mode == 1 ? observedNumber(state, "rotation", .4) : JSValue(.4)}});
        for (const auto* key : {"matrix", "offset", "repeat", "rotation", "center"})
            accessor(receiver, key, store, state, "texture");
    }
    const auto fn = receiver.get(name);
    const auto result = attempt([&] {
        auto value = fn.call(receiver, args);
        return value.sameValue(receiver)                 ? JSValue("self")
               : kind == 6 && value.sameValue(targetRGB) ? JSValue("target")
                                                         : value;
    });
    return O{{"kind", "observed"},
             {"method", name},
             {"mode", mode},
             {"result", result},
             {"stores", encode(stores)},
             {"trace", encode(state.get("log"))},
             {"name", encode(fn.get("name"))},
             {"length", encode(fn.get("length"))},
             {"constructible", fn.isConstructible()}};
}
inline JsonValue reentry(unsigned mode) {
    auto state = object({{"acted", false}, {"log", array()}, {"mode", 0}}),
         source = color({.1, .2, .3}), inner = color({.8, .7, .6}), target = color(),
         innerTarget = color();
    const auto original = source.get("getRGB");
    JSPropertyDescriptor d;
    d.configurable = d.enumerable = true;
    d.setter = JSValue::function(
        +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            auto state = c[0];
            data(state, "r", args[0]);
            if (!state.get("acted").toBoolean()) {
                data(state, "acted", true);
                c[1].call(c[2], {c[3], c[4].strictEquals(0) ? JSValue(LinearSRGBColorSpace)
                                                            : JSValue(SRGBColorSpace)});
                if (c[4].strictEquals(2))
                    throw JSRangeError("nested");
            }
            return {};
        },
        {state, original, inner, innerTarget, mode});
    target.defineProperty("r", d);
    const auto result = attempt([&] {
        return original.call(source, {target, LinearSRGBColorSpace}).sameValue(target)
                   ? JSValue("target")
                   : JSValue();
    });
    return O{{"kind", "reentry"},
             {"mode", mode},
             {"result", result},
             {"r", encode(state.get("r"))},
             {"g", encode(target.get("g"))},
             {"b", encode(target.get("b"))},
             {"inner", encode(values(innerTarget, "rgb"))}};
}
inline JsonValue chaining(unsigned mode) {
    auto state = object({{"log", array()}, {"mode", 0}}), self = matrix(), other = matrix(),
         tex = texture();
    JSValue receiver = self, fn;
    Args args;
    if (mode < 2) {
        fn = self.get(mode ? "multiply" : "premultiply");
        callback(self, "multiplyMatrices", state, "replacement");
        args = {other};
    } else if (mode == 2) {
        fn = self.get("setUvTransform");
        callback(self, "set", state, "ignored");
        args = {.1, .2, 2, 3, 0, .4, .5};
    } else {
        receiver = tex;
        fn = tex.get("updateMatrix");
        callback(tex.get("matrix"), "setUvTransform", state, "ignored");
    }
    const auto result = attempt([&] {
        const auto value = fn.call(receiver, args);
        return value.sameValue(receiver) ? JSValue("self") : value;
    });
    return O{{"kind", "chaining"},
             {"mode", mode},
             {"result", result},
             {"trace", encode(state.get("log"))}};
}
inline JsonValue trig(unsigned seed) {
    double n = seed == 0   ? -0.
               : seed == 1 ? std::numeric_limits<double>::infinity()
               : seed == 2 ? std::nan("")
               : seed < 40 ? std::pow(2., int(seed) - 20) * 1.5707963267948966
                           : (double(seed) - 400) / 73.;
    return O{{"kind", "trig"},
             {"seed", seed},
             {"cos", encode(JSNumberMath::cos(n))},
             {"sin", encode(JSNumberMath::sin(n))}};
}
inline JsonValue colorProtocol(unsigned mode) {
    const auto previous = ColorManagement::workingColorSpace;
    const auto enabled = ColorManagement::enabled;
    ColorManagement::workingColorSpace = LinearSRGBColorSpace;
    ColorManagement::enabled = true;
    auto state = object({{"log", array()}, {"mode", 0}, {"acted", false}}),
         source = color({.1, .2, .3}), other = color({.4, .5, .6}), target = color(),
         innerTarget = color();
    auto prototype = source.getPrototype();
    const auto descriptor = prototype.getOwnPropertyDescriptor("copy");
    const auto original = source.get("copy"), getRGB = source.get("getRGB");
    JSPropertyDescriptor d;
    d.configurable = true;
    d.enumerable = false;
    d.getter = JSValue::function(
        +[](const JSValue& self, const Args&, const Args& c) -> JSValue {
            trace(c[0], {"getCopy"});
            data(c[0], "scratch", self);
            return c[1];
        },
        {state, JSValue::function(
                    +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                        trace(c[0], {"callCopy", args[0].sameValue(c[2])});
                        const auto result = c[1].call(self, args);
                        if (c[3].strictEquals(0))
                            return c[4];
                        if (c[3].strictEquals(1))
                            ColorManagement::workingColorSpace = SRGBColorSpace;
                        if (c[3].strictEquals(2))
                            ColorManagement::enabled = false;
                        if (c[3].strictEquals(3))
                            throw JSRangeError("copy");
                        return result;
                    },
                    {state, original, source, mode, other})});
    prototype.defineProperty("copy", d);
    JSValue space = mode == 1 ? JSValue() : JSValue(SRGBColorSpace);
    if (mode >= 4) {
        auto key = object();
        data(key, "toString",
             JSValue::function(
                 +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                     trace(c[0], {"space"});
                     if (c[1].strictEquals(5) && !c[0].get("acted").toBoolean()) {
                         data(c[0], "acted", true);
                         c[2].call(c[3], {c[4], LinearSRGBColorSpace});
                     }
                     return SRGBColorSpace;
                 },
                 {state, mode, getRGB, other, innerTarget}));
        space = key;
    }
    auto result = attempt([&] {
        return getRGB.call(source, {target, space}).sameValue(target) ? JSValue("target")
                                                                      : JSValue();
    });
    prototype.defineProperty("copy", *descriptor);
    const auto scratch = state.get("scratch");
    ColorManagement::workingColorSpace = previous;
    ColorManagement::enabled = enabled;
    return O{{"kind", "colorProtocol"},
             {"mode", mode},
             {"result", result},
             {"target", encode(values(target, "rgb"))},
             {"other", encode(values(other, "rgb"))},
             {"scratch", encode(values(scratch, "rgb"))},
             {"inner", encode(values(innerTarget, "rgb"))},
             {"trace", encode(state.get("log"))}};
}
inline JsonValue textureNative(unsigned mode) {
    auto owner = std::make_shared<Texture>();
    owner->offset.set(.2, -.3);
    owner->repeat.set(2, 3);
    owner->center.set(.4, .6);
    owner->rotation = .7;
    owner->colorSpace = SRGBColorSpace;
    auto t = nativeTexture(owner), saved = t.get("matrix"), elements = saved.get("elements");
    const auto result = invoke(t, "updateMatrix");
    if (mode == 1)
        elements.set("6", 7);
    if (mode == 2)
        owner->matrix.elements()[7] = 9;
    if (mode == 3)
        t.set("colorSpace", LinearSRGBColorSpace);
    return O{{"kind", "textureNative"},
             {"mode", mode},
             {"result", encode(result)},
             {"matrixIdentity", saved.sameValue(t.get("matrix"))},
             {"elementsIdentity", elements.sameValue(t.get("matrix").get("elements"))},
             {"elements", encode(elements)},
             {"colorSpace", encode(t.get("colorSpace"))}};
}

} // namespace MaterialsMathFixture

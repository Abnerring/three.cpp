#pragma once
#include "core/Layers.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "core/Raycaster.h"
#include "math/ColorManagement.h"
#include "renderers/shaders/UniformsUtils.h"
#include "utils.h"
#include <cmath>
#include <functional>
using LayerArgs = std::vector<JSValue>;
namespace LayersUniformsFixture {
using A = JsonValue::Array;
using O = JsonValue::Object;
inline JSValue list(std::initializer_list<JSValue> args = {}) {
    auto value = JSValue::array();
    for (const auto& arg : args)
        value.get("push").call(value, {arg});
    return value;
}
inline void log(JSValue events, JSValue value) {
    events.get("push").call(events, {value});
}
inline void data(JSValue object, const char* key, JSValue value, bool writable = true) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = writable;
    d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
inline void getter(JSValue object, const char* key, JSValue get, JSValue set = {}) {
    JSPropertyDescriptor d;
    d.getter = get;
    if (!set.isUndefined())
        d.setter = set;
    d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
inline JsonValue encode(const JSValue& value) {
    if (value.isUndefined())
        return "@undefined";
    if (value.isSymbol())
        return "@symbol";
    if (value.isCallable())
        return "@function";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n))
            return "@NaN";
        if (n == INFINITY)
            return "@Infinity";
        if (n == -INFINITY)
            return "@-Infinity";
        if (n == 0 && std::signbit(n))
            return "@-0";
        return n;
    }
    if (value.type() == JSValue::Type::Array) {
        A a;
        for (unsigned i = 0; i < value.length(); ++i)
            a.push_back(encode(value.get(std::to_string(i))));
        return a;
    }
    if (value.isObject()) {
        if (value.hasOwn("tag"))
            return O{{"tag", encode(value.get("tag"))}};
        return O{{"keys", JsonValue::Array{}}};
    }
    return value.toJSON().value_or(JsonValue());
}
inline JsonValue names(const std::vector<std::string>& keys) {
    A a;
    for (const auto& k : keys)
        a.push_back(k);
    return a;
}
inline JsonValue desc(const JSPropertyDescriptor& d) {
    O o{{"enumerable", d.enumerable.value_or(false)},
        {"configurable", d.configurable.value_or(false)}};
    if (d.value) {
        o["value"] = encode(*d.value);
        o["writable"] = d.writable.value_or(false);
    } else {
        o["get"] = d.getter && d.getter->isCallable();
        o["set"] = d.setter && d.setter->isCallable();
    }
    return o;
}
inline JsonValue snapshot(const JSValue& value) {
    O o;
    if (value.isObject())
        for (const auto& key : value.ownPropertyNames())
            o[key] = desc(*value.getOwnPropertyDescriptor(key));
    return o;
}
inline std::string attempt(const std::function<void()>& fn) {
    try {
        fn();
        return "";
    } catch (const JSTypeError&) {
        return "TypeError";
    } catch (const JSRangeError&) {
        return "RangeError";
    }
}
inline JSValue value(int index) {
    switch (index) {
    case 0:
        return {};
    case 1:
        return nullptr;
    case 2:
        return false;
    case 3:
        return true;
    case 4:
        return 0;
    case 5:
        return -0.;
    case 6:
        return 1;
    case 7:
        return -1;
    case 8:
        return 31;
    case 9:
        return 32;
    case 10:
        return 33;
    case 11:
        return -33;
    case 12:
        return 4294967295.;
    case 13:
        return 4294967296.;
    case 14:
        return NAN;
    case 15:
        return INFINITY;
    case 16:
        return -INFINITY;
    case 17:
        return " 2.9 ";
    case 18:
        return "invalid";
    case 19:
        return list();
    case 20:
        return list({1});
    case 21:
        return JSValue::object();
    case 22:
        return JSValue::symbol("k");
    default:
        return 1.9;
    }
}
inline JsonValue numeric(const char* method, int m, int n) {
    auto self = Layers().object();
    auto mask = value(m), arg = value(n);
    self.set("mask", mask);
    if (std::string(method) == "test") {
        auto other = JSValue::object();
        other.set("mask", arg);
        arg = other;
    }
    JSValue result;
    const auto error = attempt([&] { result = Layers::prototype().get(method).call(self, {arg}); });
    return O{{"kind", "numeric"},
             {"method", method},
             {"m", m},
             {"n", n},
             {"error", error},
             {"mask", encode(self.get("mask"))},
             {"result", encode(result)}};
}
inline JsonValue metadata() {
    auto ctor = Layers::constructor(), proto = Layers::prototype();
    O methods;
    for (const auto* name :
         {"set", "enable", "enableAll", "toggle", "disable", "disableAll", "test", "isEnabled"}) {
        auto fn = proto.get(name);
        methods[name] = O{{"name", encode(fn.get("name"))},
                          {"length", encode(fn.get("length"))},
                          {"own", snapshot(fn)},
                          {"desc", desc(*proto.getOwnPropertyDescriptor(name))},
                          {"new", attempt([&] { fn.construct(); })}};
    }
    return O{{"kind", "metadata"},
             {"ctor", snapshot(ctor)},
             {"prototype", names(proto.ownPropertyNames())},
             {"methods", methods},
             {"instance", snapshot(Layers().object())},
             {"call", attempt([&] { ctor.call(); })}};
}
inline JsonValue construct(int mode) {
    auto ctor = Layers::constructor(), events = list(), prototype = JSValue::object();
    auto other = JSValue::constructor(
        +[](const JSValue&, const LayerArgs&, const LayerArgs&) -> JSValue { return {}; }, {},
        "Other", 0, JSValue::ConstructorKind::Callable);
    other.set("prototype", mode == 4 ? JSValue(7) : prototype);
    if (mode == 1 || mode == 2 || mode == 5)
        getter(prototype, "mask",
               JSValue::function(+[](const JSValue&, const LayerArgs&,
                                     const LayerArgs&) -> JSValue { return 12; }),
               JSValue::function(
                   +[](const JSValue& self, const LayerArgs& a, const LayerArgs& c) -> JSValue {
                       log(c[0], list({"setter", a[0], self.getPrototype().sameValue(c[1])}));
                       if (c[2].strictEquals(2))
                           throw JSRangeError("setter");
                       if (c[2].strictEquals(5)) {
                           auto receiver = self;
                           data(receiver, "tag", "changed");
                       }
                       return {};
                   },
                   {events, prototype, mode}));
    if (mode == 3)
        data(prototype, "mask", 12, false);
    JSValue result;
    const auto error = attempt([&] { result = ctor.construct({}, other); });
    return O{{"kind", "construct"},
             {"mode", mode},
             {"error", error},
             {"result", snapshot(result)},
             {"samePrototype", result.isObject() && result.getPrototype().sameValue(prototype)},
             {"events", encode(events)}};
}
inline JsonValue dynamic(const char* name, int leftMode, int argMode) {
    auto self = JSValue::object(), other = JSValue::object(), events = list();
    auto box = JSValue::object();
    box.set("tag", "mask-conversion");
    box.set("valueOf", JSValue::function(
                           +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                               log(c[0], "mask.valueOf");
                               if (c[1].strictEquals(7))
                                   throw JSRangeError("mask conversion");
                               return 5;
                           },
                           {events, leftMode}));
    getter(self, "mask",
           JSValue::function(
               +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                   log(c[0], "mask.get");
                   auto self = c[1], other = c[2];
                   const int mode = int(c[3].toNumber());
                   if (mode == 1)
                       throw JSRangeError("mask getter");
                   if (mode == 2)
                       other.set("mask", 1);
                   if (mode == 3)
                       self.setPrototype(JSValue::object());
                   if (mode == 4)
                       self.erase("mask");
                   if (mode == 5)
                       Layers::prototype().get("enableAll").call(self);
                   return mode >= 6 ? c[4] : JSValue(5);
               },
               {events, self, other, leftMode, box}),
           JSValue::function(
               +[](const JSValue&, const LayerArgs& a, const LayerArgs& c) -> JSValue {
                   log(c[0], list({"mask.set", a[0]}));
                   if (c[1].strictEquals(5))
                       throw JSRangeError("setter");
                   return {};
               },
               {events, leftMode}));
    auto arg = JSValue::object();
    arg.set("tag", "arg");
    arg.set("valueOf", JSValue::function(
                           +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                               log(c[0], "arg.valueOf");
                               auto self = c[1];
                               const int mode = int(c[2].toNumber());
                               if (mode == 1)
                                   throw JSRangeError("arg conversion");
                               if (mode == 2)
                                   data(self, "mask", 99);
                               if (mode == 3)
                                   Layers::prototype().get("disableAll").call(self);
                               if (mode == 4)
                                   return JSValue::object();
                               if (mode == 5)
                                   return JSValue::symbol("x");
                               return 2;
                           },
                           {events, self, argMode}));
    other.set("mask", arg);
    if (argMode == 6)
        getter(other, "mask",
               JSValue::function(
                   +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                       log(c[0], "other.mask.get");
                       throw JSRangeError("right getter");
                       return {};
                   },
                   {events}));
    JSValue result;
    const auto error = attempt([&] {
        result =
            Layers::prototype().get(name).call(self, {std::string(name) == "test" ? other : arg});
    });
    return O{{"kind", "dynamic"},     {"method", name},          {"left", leftMode},
             {"arg", argMode},        {"error", error},          {"result", encode(result)},
             {"own", snapshot(self)}, {"events", encode(events)}};
}
inline JsonValue native(int mode) {
    auto owner = std::make_shared<Object3D>();
    auto wrapper = nativeObject(owner);
    auto original = wrapper.get("layers"), other = Layers().object();
    other.set("mask", 5);
    if (mode == 0)
        owner->layers.mask = -1;
    if (mode == 1)
        original.set("mask", "9");
    if (mode == 2)
        wrapper.set("layers", other);
    if (mode == 3)
        owner->layers = Layers::fromObject(other);
    if (mode == 4) {
        wrapper.set("layers", other);
        original.set("mask", 3);
    }
    if (mode == 5)
        wrapper.set("layers", nullptr);
    if (mode == 6)
        wrapper.set("layers", 7);
    if (mode == 7) {
        auto plain = JSValue::object();
        plain.set("mask", "3");
        wrapper.set("layers", plain);
    }
    if (mode == 8) {
        owner->layers.enable(31);
        owner->layers.enable(0);
    }
    JSValue mask;
    const auto error = attempt([&] { mask = owner->layers.mask.value(); });
    return O{{"kind", "native"},
             {"mode", mode},
             {"error", error},
             {"mask", encode(mask)},
             {"original", encode(original.get("mask"))},
             {"sameOriginal", wrapper.get("layers").sameValue(original)},
             {"sameOther", wrapper.get("layers").sameValue(other)}};
}
inline JsonValue uniformsMetadata() {
    auto legacy = nativeUniformsUtils();
    O functions;
    for (const auto& [name, fn] : std::initializer_list<std::pair<const char*, JSValue>>{
             {"cloneUniforms", nativeCloneUniforms()},
             {"mergeUniforms", nativeMergeUniforms()},
             {"cloneUniformsGroups", nativeCloneUniformsGroups()},
             {"getUnlitUniformColorSpace", nativeGetUnlitUniformColorSpace()}})
        functions[name] = snapshot(fn);
    return O{{"kind", "uniforms-metadata"},
             {"legacy", snapshot(legacy)},
             {"keys", names(legacy.ownPropertyNames())},
             {"cloneIdentity", legacy.get("clone").sameValue(nativeCloneUniforms())},
             {"mergeIdentity", legacy.get("merge").sameValue(nativeMergeUniforms())},
             {"functions", functions}};
}
inline JsonValue uniforms(int mode) {
    auto legacy = nativeUniformsUtils();
    auto old = legacy.get("clone");
    auto source = JSValue::object();
    auto uniform = JSValue::object();
    uniform.set("value", 7);
    source.set("u", uniform);
    auto events = list();
    JSValue fn = nativeCloneUniforms(), result;
    if (mode == 1)
        fn = nativeMergeUniforms();
    if (mode == 2)
        fn = nativeCloneUniformsGroups();
    if (mode == 3 || mode == 4)
        fn = nativeGetUnlitUniformColorSpace();
    if (mode == 5 || mode == 6) {
        auto replacement = JSValue::constructor(
            +[](const JSValue& self, const LayerArgs& a, const LayerArgs& c) -> JSValue {
                log(c[0], list({"replacement", self.sameValue(c[1]), a.size()}));
                return "replace";
            },
            {events, legacy}, "replacement", 1, JSValue::ConstructorKind::Callable);
        legacy.set("clone", replacement);
        fn = mode == 5 ? legacy.get("clone") : nativeCloneUniforms();
    }
    if (mode >= 7) {
        auto texture = JSValue::object();
        texture.set("isTexture", true);
        texture.set("isRenderTargetTexture", true);
        uniform.set("value", texture);
        ThreeUtils::setConsoleFunction(JSValue::function(
            +[](const JSValue&, const LayerArgs& a, const LayerArgs& c) -> JSValue {
                for (const auto& v : a)
                    log(c[0], v);
                if (c[1].strictEquals(8))
                    throw JSRangeError("warning");
                if (c[1].strictEquals(9)) {
                    auto uniform = c[2];
                    uniform.set("extra", 9);
                }
                return {};
            },
            {events, mode, uniform}));
    }
    JSValue input = source;
    if (mode == 1)
        input = list({source});
    if (mode == 2) {
        auto cloneable = JSValue::object();
        cloneable.set("clone", JSValue::function(+[](const JSValue&, const LayerArgs&,
                                                     const LayerArgs&) -> JSValue { return 19; }));
        input = list({cloneable});
    }
    if (mode == 3 || mode == 4) {
        input = JSValue::object();
        input.set("getRenderTarget",
                  JSValue::function(+[](const JSValue&, const LayerArgs&,
                                        const LayerArgs&) -> JSValue { return nullptr; }));
        input.set("outputColorSpace", "test-space");
    }
    const auto error =
        attempt([&] { result = (mode == 4 ? fn.construct({input}) : fn.call(legacy, {input})); });
    ThreeUtils::setConsoleFunction({});
    legacy.set("clone", old);
    JsonValue output;
    if (result.isObject() && result.has("u"))
        output = encode(result.get("u").get("value"));
    else
        output = encode(result);
    return O{{"kind", "uniforms"},
             {"mode", mode},
             {"error", error},
             {"result", output},
             {"events", encode(events)}};
}
inline JsonValue borrow(const char* method, int mode) {
    auto self = value(mode);
    JSValue result;
    const auto error = attempt([&] {
        result = Layers::prototype().get(method).call(
            self, {std::string(method) == "test" ? JSValue::object() : JSValue(2)});
    });
    return O{{"kind", "borrow"},
             {"method", method},
             {"mode", mode},
             {"error", error},
             {"result", encode(result)}};
}
inline JsonValue namespaceCall(int mode) {
    auto legacy = nativeUniformsUtils(), events = list(), source = JSValue::object(),
         uniform = JSValue::object();
    uniform.set("value", 9);
    source.set("u", uniform);
    const auto key = mode < 2 ? "clone" : "merge";
    auto original = legacy.getOwnPropertyDescriptor(key);
    getter(legacy, key,
           JSValue::function(
               +[](const JSValue& self, const LayerArgs&, const LayerArgs& c) -> JSValue {
                   log(c[0], list({"getter", self.sameValue(c[1])}));
                   if (c[2].strictEquals(1))
                       throw JSRangeError("getter");
                   auto legacy = c[1];
                   if (c[2].strictEquals(2))
                       legacy.set("clone", JSValue::function(+[](const JSValue&, const LayerArgs&,
                                                                 const LayerArgs&) -> JSValue {
                                      throw JSTypeError("replacement must not be called");
                                      return {};
                                  }));
                   return c[3];
               },
               {events, legacy, mode, mode < 2 ? nativeCloneUniforms() : nativeMergeUniforms()}));
    JSValue result;
    const auto error = attempt([&] {
        result = UniformsUtils.get(key).call(UniformsUtils.object(),
                                             {mode < 2 ? source : list({source})});
    });
    legacy.defineProperty(key, *original);
    legacy.set("clone", nativeCloneUniforms());
    return O{{"kind", "namespace"},
             {"mode", mode},
             {"error", error},
             {"result", result.isObject() ? encode(result.get("u").get("value")) : encode(result)},
             {"events", encode(events)}};
}
inline JsonValue dispatch(int mode) {
    auto left = std::make_shared<Object3D>(), right = std::make_shared<Object3D>();
    left->layers.mask = 2;
    right->layers.mask = 1;
    auto l = nativeObject(left), r = nativeObject(right), original = left->layers.object(),
         events = list(), replacement = Layers().object();
    replacement.set("mask", 2);
    getter(original, "test",
           JSValue::function(
               +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                   log(c[0], "test.get");
                   auto right = c[1], left = c[2];
                   right.set("layers", c[3]);
                   if (c[4].strictEquals(1))
                       left.set("layers", Layers().object());
                   if (c[4].strictEquals(2))
                       throw JSRangeError("getter");
                   return c[5];
               },
               {events, r, l, replacement, mode, Layers::prototype().get("test")}));
    JSValue result;
    const auto error = attempt([&] { result = left->layers.test(right->layers); });
    return O{{"kind", "dispatch"},
             {"mode", mode},
             {"error", error},
             {"result", encode(result)},
             {"leftMask", encode(left->layers.mask.value())},
             {"rightMask", encode(right->layers.mask.value())},
             {"events", encode(events)}};
}
inline JsonValue copyReentry(int mode) {
    auto source = std::make_shared<Object3D>(), destination = std::make_shared<Object3D>();
    auto sourceLayer = source->layers.object(), oldTarget = destination->layers.object(),
         replacement = Layers().object(), wrapper = nativeObject(destination), events = list();
    replacement.set("mask", 5);
    getter(sourceLayer, "mask",
           JSValue::function(
               +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                   log(c[0], "source.mask.get");
                   auto wrapper = c[1];
                   if (c[2].toNumber() > 0)
                       wrapper.set("layers", c[2].strictEquals(2) ? JSValue(nullptr) : c[3]);
                   if (c[2].strictEquals(3))
                       throw JSRangeError("mask getter");
                   return 19;
               },
               {events, wrapper, mode, replacement}));
    const auto error = attempt([&] { destination->copy(*source, false); });
    JSValue current;
    const auto read = attempt([&] { current = destination->layers.mask.value(); });
    return O{{"kind", "copy-reentry"},
             {"mode", mode},
             {"error", error},
             {"read", read},
             {"oldMask", encode(oldTarget.get("mask"))},
             {"current", encode(current)},
             {"events", encode(events)}};
}
inline JsonValue unlitWorking(int branch, int mode, int index) {
    auto management = nativeColorManagement(), events = list(), renderer = JSValue::object(),
         target = JSValue::object(), texture = JSValue::object(), replacement = JSValue::object(),
         state = JSValue::object();
    const auto original = *management.getOwnPropertyDescriptor("workingColorSpace");
    struct RestoreWorking {
        JSValue management;
        JSPropertyDescriptor descriptor;
        ~RestoreWorking() { management.defineProperty("workingColorSpace", descriptor); }
    } restore{management, original};
    replacement.set("tag", "replacement");
    state.set("reads", 0);
    auto working = value(index);
    if (index == 24)
        working = JSValue::function(+[](const JSValue&, const LayerArgs&, const LayerArgs&) -> JSValue {
            throw JSRangeError("working value must not be called");
        });
    if (index == 25) {
        working = JSValue::object();
        working.set("tag", "working");
        for (const auto* name : {"valueOf", "toString"})
            working.set(name, JSValue::function(
                +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                    log(c[0], c[1]);
                    throw JSRangeError("working value must not be converted");
                }, {events, name}));
    }
    data(management, "workingColorSpace", working);
    if (mode != 0)
        getter(management, "workingColorSpace", JSValue::function(
            +[](const JSValue& self, const LayerArgs&, const LayerArgs& c) -> JSValue {
                const auto count = c[4].get("reads").toNumber();
                auto state = c[4];
                state.set("reads", count + 1);
                log(c[0], list({"working.get", self.sameValue(c[1]), count}));
                const auto mode = int(c[5].toNumber());
                if (mode == 2) throw JSRangeError("working getter");
                if (mode >= 3 && mode <= 5 && count == 0) {
                    auto nested = JSValue::object(), nestedTarget = JSValue::object(),
                         nestedTexture = JSValue::object();
                    nestedTarget.set("isXRRenderTarget", mode == 5);
                    nestedTexture.set("colorSpace", c[3]);
                    nestedTarget.set("texture", nestedTexture);
                    nested.set("outputColorSpace", c[3]);
                    nested.set("getRenderTarget", JSValue::function(
                        +[](const JSValue&, const LayerArgs&, const LayerArgs& captures) -> JSValue {
                            log(captures[0], "nested.target");
                            return captures[1];
                        }, {c[0], mode == 4 ? JSValue(nullptr) : nestedTarget}));
                    const auto result = nativeGetUnlitUniformColorSpace().call({}, {nested});
                    log(c[0], list({"nested.result", result.sameValue(c[3])}));
                }
                if (mode == 6) data(c[1], "workingColorSpace", c[3]);
                return mode == 3 && count > 0 ? c[3] : c[2];
            }, {events, management, working, replacement, state, mode}));
    getter(renderer, "getRenderTarget", JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
            log(c[0], "target.method.get");
            return c[1];
        }, {events, JSValue::function(
            +[](const JSValue& self, const LayerArgs&, const LayerArgs& c) -> JSValue {
                log(c[0], list({"target.call", self.sameValue(c[1])}));
                if (c[5].strictEquals(7)) data(c[3], "workingColorSpace", c[4]);
                return c[2];
            }, {events, renderer, branch == 1 ? JSValue(nullptr) : target, management,
                replacement, mode})}));
    getter(target, "isXRRenderTarget", JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
            log(c[0], "xr.get");
            return c[1].strictEquals(3) ? JSValue(1) : JSValue(c[1].strictEquals(2));
        }, {events, branch}));
    getter(target, "texture", JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
            log(c[0], "texture.get"); return c[1];
        }, {events, texture}));
    getter(texture, "colorSpace", JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
            log(c[0], "texture.colorSpace.get"); return c[1];
        }, {events, replacement}));
    getter(renderer, "outputColorSpace", JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
            log(c[0], "output.get"); return c[1];
        }, {events, replacement}));
    A calls;
    for (int i = 0; i < 2; ++i) {
        JSValue result;
        const auto error = attempt([&] { result = nativeGetUnlitUniformColorSpace().call({}, {renderer}); });
        calls.push_back(O{{"error", error}, {"value", encode(result)},
                          {"workingIdentity", result.sameValue(working)},
                          {"replacementIdentity", result.sameValue(replacement)}});
    }
    return O{{"kind", "unlit-working"}, {"branch", branch}, {"mode", mode}, {"index", index},
             {"calls", calls}, {"events", encode(events)}};
}
inline A run() {
    A cases;
    cases.push_back(metadata());
    for (int mode = 0; mode < 6; ++mode)
        cases.push_back(construct(mode));
    for (const auto* method : {"set", "enable", "toggle", "disable", "test", "isEnabled"})
        for (int m = 0; m < 24; ++m)
            for (int n = 0; n < 24; ++n)
                cases.push_back(numeric(method, m, n));
    for (const auto* method : {"set", "enable", "toggle", "disable", "test", "isEnabled"})
        for (int left = 0; left < 8; ++left)
            for (int arg = 0; arg < 7; ++arg)
                cases.push_back(dynamic(method, left, arg));
    for (int mode = 0; mode < 9; ++mode)
        cases.push_back(native(mode));
    for (const auto* method :
         {"set", "enable", "enableAll", "toggle", "disable", "disableAll", "test", "isEnabled"})
        for (int mode = 0; mode < 24; ++mode)
            cases.push_back(borrow(method, mode));
    for (int mode = 0; mode < 3; ++mode)
        cases.push_back(namespaceCall(mode));
    for (int mode = 0; mode < 3; ++mode)
        cases.push_back(dispatch(mode));
    for (int mode = 0; mode < 4; ++mode)
        cases.push_back(copyReentry(mode));
    cases.push_back(uniformsMetadata());
    for (int mode = 0; mode < 10; ++mode)
        cases.push_back(uniforms(mode));
    for (int branch = 0; branch < 4; ++branch) {
        for (int mode = 0; mode < 2; ++mode)
            for (int index = 0; index < 26; ++index)
                cases.push_back(unlitWorking(branch, mode, index));
        for (int mode = 2; mode < 8; ++mode)
            cases.push_back(unlitWorking(branch, mode, 25));
    }
    return cases;
}
} // namespace LayersUniformsFixture

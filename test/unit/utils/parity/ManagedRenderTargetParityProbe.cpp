#include "core/RenderTarget.h"
#include "renderers/common/CubeRenderTarget.h"
#include "textures/CubeTexture.h"
#include "core/NativeArray.h"
#include "math/Vector3.h"
#include "utils.h"
#include <cmath>
#include <iostream>

namespace {
using Args = std::vector<JSValue>; using A = JsonValue::Array; using O = JsonValue::Object;
JSValue call(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
JSValue array(const Args& values = {}) { auto result = JSValue::array(); for (const auto& value : values) call(result, "push", {value}); return result; }
JSValue object(const std::initializer_list<std::pair<const char*, JSValue>>& values = {}) { auto result = JSValue::object(); for (const auto& [key, value] : values) result.set(key, value); return result; }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "#undefined"; if (value.isNull()) return nullptr;
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Number) { const auto n = value.number(); if (std::isnan(n)) return "#NaN"; if (std::isinf(n)) return n < 0 ? "#-Infinity" : "#Infinity"; if (n == 0 && std::signbit(n)) return "#-0"; return n; }
    if (value.isCallable()) return "#function"; if (value.isSymbol()) return "#symbol";
    if (value.hasOwn("tag")) return O{{"ref", encode(value.get("tag"))}};
    if (value.type() == JSValue::Type::Array) { A result; for (unsigned i = 0; i < value.length(); ++i) result.push_back(encode(value.get(JSValue(i)))); return result; }
    O result; for (const auto& key : value.keys()) result[key] = encode(value.get(key)); return result;
}
void row(const char* kind, JsonValue value) { std::cout << JsonValue(O{{"kind", kind}, {"value", std::move(value)}}).dump() << '\n'; }
JsonValue names(const JSValue& value) { A result; for (const auto& key : value.ownPropertyNames()) result.emplace_back(key); return result; }
JsonValue attempt(const std::function<JSValue()>& fn) { try { return encode(fn()); } catch (const JSRangeError&) { return "RangeError"; } catch (const JSTypeError&) { return "TypeError"; } }
JsonValue textureState(const JSValue& texture, const JSValue& target) {
    const auto image = texture.get("image");
    A options; for (const auto* key : {"mapping", "wrapS", "wrapT", "magFilter", "minFilter", "format", "type", "anisotropy", "colorSpace", "flipY", "generateMipmaps", "mipmapsAutoUpdate", "internalFormat", "unpackAlignment"}) options.push_back(encode(texture.get(key)));
    return O{{"size", A{encode(image.get("width")), encode(image.get("height")), encode(image.get("depth"))}}, {"version", encode(texture.get("version"))}, {"sourceVersion", encode(texture.get("source").get("version"))},
        {"flags", A{encode(texture.get("isRenderTargetTexture")), encode(texture.get("isArrayTexture")), encode(texture.get("isData3DTexture")), encode(texture.get("isCubeTexture"))}}, {"owner", texture.get("renderTarget").strictEquals(target)}, {"options", options}};
}
JsonValue state(const JSValue& target) {
    A textures; const auto values = target.get("textures"); for (unsigned i = 0; i < values.length(); ++i) textures.push_back(textureState(values.get(JSValue(i)), target));
    A flags; for (const auto* key : {"scissorTest", "depthBuffer", "stencilBuffer", "resolveColorBuffer", "resolveDepthBuffer", "resolveStencilBuffer", "storeMultisampledColorBuffer", "storeMultisampledDepthBuffer", "storeMultisampledStencilBuffer", "multiview", "useArrayDepthTexture"}) flags.push_back(encode(target.get(key)));
    const auto first = values.get("0"), second = values.get("1");
    return O{{"size", A{encode(target.get("width")), encode(target.get("height")), encode(target.get("depth"))}}, {"scissor", encode(call(target.get("scissor"), "toArray"))}, {"viewport", encode(call(target.get("viewport"), "toArray"))}, {"flags", flags}, {"samples", encode(target.get("samples"))}, {"textures", textures},
        {"sameSource", values.length() > 1 && first.get("source").strictEquals(second.get("source"))}, {"sameImage", values.length() > 1 && first.get("image").strictEquals(second.get("image"))}};
}
void metadata() {
    const auto ctor = nativeRenderTargetConstructor(), cube = nativeCubeRenderTargetConstructor();
    for (const auto& current : {ctor, cube}) {
        A arities; const auto prototype = current.get("prototype");
        for (const auto& key : prototype.ownPropertyNames()) { const auto d = prototype.getOwnPropertyDescriptor(key); if (d->value && d->value->isCallable()) arities.emplace_back(A{key, encode(d->value->get("length"))}); }
        row("constructor metadata", A{encode(current.get("name")), encode(current.get("length")), encode(current.getPrototype().get("name")), names(prototype), arities});
    }
    const auto target = ctor.construct(); row("target own keys", names(target));
    for (const auto& current : {nativeTextureConstructor(), nativeTextureSourceConstructor(), nativeCubeTextureConstructor()}) { auto value = current.construct(); row("resource own keys", names(value)); }
    const auto own = nativeTextureConstructor().construct().getOwnPropertyDescriptor("id"); row("texture id descriptor", A{own->writable.value_or(false), own->enumerable.value_or(false), own->configurable.value_or(false)});
    row("canonical identity", A{ctor.strictEquals(nativeRenderTargetConstructor()), cube.strictEquals(nativeCubeRenderTargetConstructor()), nativeTextureConstructor().strictEquals(nativeTextureConstructor()), nativeTextureSourceConstructor().strictEquals(nativeTextureSourceConstructor())});
}
void construction() {
    const auto ctor = nativeRenderTargetConstructor();
    for (const auto& count : Args{JSValue{}, nullptr, false, true, -1, 0, 1, 2, 1.5, "2", "bad", std::nan("")}) {
        auto options = object({{"count", count}, {"depth", 2}, {"minFilter", 1008}, {"flipY", true}, {"internalFormat", "R8"}}); row("construct count", state(ctor.construct({3, 4, options})));
    }
    for (const auto& value : Args{JSValue{}, nullptr, false, true, 0, -0.0, -1, 2.5, "5", "", std::nan(""), std::numeric_limits<double>::infinity(), object({{"tag", "raw"}})}) {
        auto options = object({{"count", 0}}); for (const auto* key : {"depth", "depthBuffer", "stencilBuffer", "resolveColorBuffer", "resolveDepthBuffer", "resolveStencilBuffer", "storeMultisampledColorBuffer", "storeMultisampledDepthBuffer", "storeMultisampledStencilBuffer", "samples", "multiview", "useArrayDepthTexture"}) options.set(key, value);
        row("construct arbitrary values", state(ctor.construct({value, value, options})));
    }
    for (const auto& options : Args{nullptr, false, 0, "abc", array({1, 2})}) row("construct primitive options", state(ctor.construct({3, 4, options})));
}
void sizeCases() {
    const auto ctor = nativeRenderTargetConstructor();
    for (unsigned mode = 0; mode < 12; ++mode) {
        auto target = ctor.construct({3, 4, object({{"count", 2}})}), trace = JSValue::array();
        if (mode == 1) target.set("width", "3"); if (mode == 2) target.set("depth", -0.0);
        if (mode == 3) target.get("textures").get("0").set("isData3DTexture", true);
        if (mode == 4) target.get("textures").get("0").set("isData3DTexture", 1);
        const auto listener = JSValue::function([](const JSValue& self, const Args& args, const Args& captures) -> JSValue { auto trace = captures[0]; auto target = self; call(trace, "push", {array({"dispose", args[0].get("target").strictEquals(self), target.get("width"), target.get("height"), target.get("depth")})}); const auto mode = captures[1].number(); if (mode == 5) throw JSTypeError("stop"); if (mode == 6) target.set("viewport", object({{"set", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSTypeError("viewport"); })}})); if (mode == 7) target.set("width", 12); return {}; }, {trace, mode}, "listener", 1);
        call(target, "addEventListener", {"dispose", listener});
        const auto width = mode == 8 ? JSValue("5") : mode == 9 ? JSValue(std::nan("")) : JSValue(3);
        const auto depth = mode == 10 ? JSValue("2") : mode == 11 ? JSValue(nullptr) : mode >= 3 ? JSValue(2) : JSValue(1);
        const auto result = attempt([&] { return call(target, "setSize", {width, 4, depth}); });
        // A replaced viewport deliberately has no toArray protocol.
        if (mode == 6) { row("setSize", A{result, encode(trace), encode(target.get("width")), encode(target.get("textures").get("0").get("image"))}); continue; }
        row("setSize", A{result, encode(trace), state(target)});
    }
}
void depthCases() {
    const auto ctor = nativeRenderTargetConstructor();
    for (unsigned mode = 0; mode < 10; ++mode) {
        auto target = ctor.construct(), other = ctor.construct(), first = object({{"tag", "first"}, {"renderTarget", nullptr}}), next = object({{"tag", "next"}, {"renderTarget", mode == 1 ? other : JSValue(nullptr)}});
        target.set("depthTexture", first);
        if (mode == 2) first.set("renderTarget", other);
        if (mode == 3) next.erase("renderTarget");
        if (mode == 4) next = JSValue{}; if (mode == 5) next = nullptr; if (mode == 6) next = first;
        if (mode == 7) target.set("_depthTexture", JSValue{});
        if (mode == 8) target.set("_depthTexture", 4);
        if (mode == 9) { JSPropertyDescriptor d; d.value = nullptr; d.writable = false; d.configurable = true; next.defineProperty("renderTarget", d); }
        const auto result = attempt([&] { target.set("depthTexture", next); return JSValue{}; });
        auto owner = [&](const JSValue& value) -> JsonValue { if (value.isNull() || value.isUndefined()) return encode(value); const auto ref = value.get("renderTarget"); return ref.strictEquals(target) ? JsonValue("target") : ref.strictEquals(other) ? JsonValue("other") : encode(ref); };
        row("depthTexture", A{result, encode(target.get("depthTexture")), owner(first), owner(next)});
    }
}
void copyCases() {
    const auto ctor = nativeRenderTargetConstructor();
    for (unsigned mode = 0; mode < 12; ++mode) {
        auto source = ctor.construct({4, 5, object({{"count", 2}, {"depth", 2}})}), destination = ctor.construct({9, 10}), trace = JSValue::array();
        auto original = source.get("texture"); const auto clone = original.get("clone"), old = destination.get("textures"); source.set("samples", 7);
        original.set("clone", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue { auto source = c[0], destination = c[1], trace = c[2]; const auto mode = c[3].number(); call(trace, "push", {array({"clone", source.get("textures").get("length"), destination.get("textures").get("length")})}); if (mode == 1) source.get("textures").set("0", nativeTextureConstructor().construct({object({{"width", 13}, {"height", 14}, {"depth", 15}})})); if (mode == 2) source.get("textures").set("length", 0); if (mode == 3) call(source.get("textures"), "push", {nativeTextureConstructor().construct()}); if (mode == 4) destination.set("textures", array({nativeTextureConstructor().construct()})); if (mode == 5) return self; if (mode == 6) { destination.set("width", 31); throw JSTypeError("stop"); } if (mode == 7) source.get("textures").erase("1"); if (mode == 8) destination.set("textures", source.get("textures")); return c[4].call(self); }, {source, destination, trace, mode, clone}, "clone", 0));
        if (mode == 9) { auto depth = nativeTextureConstructor().construct(); source.set("depthTexture", depth); }
        if (mode == 10) { auto depth = nativeTextureConstructor().construct(); depth.set("renderTarget", destination); source.set("depthTexture", depth); }
        if (mode == 11) source = destination;
        const auto result = attempt([&] { const auto value = call(destination, "copy", {source}); return value.strictEquals(destination); });
        const auto targetTextures = destination.get("textures"); A textures;
        for (unsigned i = 0; i < targetTextures.length(); ++i) { auto t = targetTextures.get(JSValue(i)); if (t.isUndefined()) textures.emplace_back("hole"); else textures.push_back(textureState(t, destination)); }
        row("copy", A{result, encode(trace), encode(destination.get("width")), encode(destination.get("height")), encode(destination.get("depth")), encode(destination.get("samples")), textures, old.strictEquals(targetTextures), encode(old.get("length")), source.get("depthTexture").strictEquals(destination.get("depthTexture"))});
    }
}
void cubeCases() {
    const auto ctor = nativeCubeRenderTargetConstructor();
    for (const auto& count : Args{0, 1, 2}) {
        const auto target = ctor.construct({3, object({{"count", count}, {"depth", 4}})}), texture = target.get("texture"), images = texture.get("images");
        row("cube constructor", A{state(target), encode(images.get("length")), images.get("0").strictEquals(images.get("5")), encode(images.get("0")), texture.get("renderTarget").isNull(), texture.getPrototype().strictEquals(nativeCubeTextureConstructor().get("prototype"))});
        row("cube clone", state(call(target, "clone")));
    }
    for (unsigned mode = 0; mode < 10; ++mode) {
        auto target = ctor.construct(), trace = JSValue::array(), renderer = object(), previous = object({{"tag", "previous"}});
        renderer.set("getRenderTarget", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"get"}); return c[1]; }, {trace, previous}));
        renderer.set("setRenderTarget", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { call(c[0], "push", {array({"set", a[0].strictEquals(c[1]) ? JSValue("target") : a[0], a.size() > 1 ? a[1] : JSValue{}})}); if (c[2].number() == 1 && a.size() > 1 && a[1].strictEquals(2)) throw JSTypeError("set"); return {}; }, {trace, target, mode}));
        renderer.set("clear", JSValue::function([](const JSValue&, const Args& a, const Args& c) -> JSValue { call(c[0], "push", {array({"clear", array(a)})}); if (c[1].number() == 2) throw JSTypeError("clear"); return {}; }, {trace, mode}));
        Args values{renderer}; if (mode >= 3) values = {renderer, mode == 3 ? JSValue{} : mode == 4 ? JSValue(nullptr) : JSValue("color"), false, 0};
        row("cube clear", A{attempt([&] { return call(target, "clear", values); }), encode(trace)});
    }
}
void protocols() {
    const auto ctor = nativeRenderTargetConstructor();
    for (const auto& resource : {nativeTextureConstructor(), nativeTextureSourceConstructor(), nativeCubeTextureConstructor()}) row("resource prototype", A{encode(resource.get("name")), names(resource.get("prototype"))});
    for (unsigned mode = 0; mode < 8; ++mode) {
        auto target = ctor.construct({3, 4, object({{"count", 2}})}), trace = JSValue::array(), options = object(), counter = object({{"reads", 0}});
        for (const auto* key : {"mapping", "minFilter", "flipY", "internalFormat"}) {
            JSPropertyDescriptor d; d.enumerable = d.configurable = true;
            d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { auto counter = c[1]; const auto n = counter.get("reads").number(); counter.set("reads", n + 1); call(c[0], "push", {array({"get", c[2], n})}); return c[2].strictEquals("internalFormat") ? JSValue("R8") : JSValue(n); }, {trace, counter, key}); options.defineProperty(key, d);
        }
        auto first = target.get("texture"); const auto base = first.get("setValues");
        first.set("setValues", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            auto target = c[0], trace = c[1], values = args[0]; call(trace, "push", {array({"setValues", JSValue::fromJSON(encode(values))})});
            const auto mode = c[2].number(); if (mode != 6) c[3].call(self, args);
            if (mode == 1) target.get("textures").set("length", 0);
            if (mode == 2) call(target.get("textures"), "push", {nativeTextureConstructor().construct({object({{"width", 7}, {"height", 8}, {"depth", 1}})})});
            if (mode == 3) target.set("textures", array({self, nativeTextureConstructor().construct({object({{"width", 7}, {"height", 8}, {"depth", 1}})})}));
            if (mode == 4) throw JSTypeError("stop"); if (mode == 5) values.set("mapping", "changed"); if (mode == 7) target.get("textures").set("length", 1);
            return {};
        }, {target, trace, mode, base}));
        row("texture options getters", A{attempt([&] { return call(target, "_setTextureOptions", {options}); }), encode(trace), state(target)});
    }
    for (const auto& value : Args{JSValue{}, nullptr, false, true, 0, -0.0, "7", object({{"tag", "dimension"}})}) {
        auto source = nativeTextureSourceConstructor().construct({object({{"width", value}, {"height", value}, {"depth", value}})}), target = nativeVector3Constructor().construct();
        row("source getSize", A{call(source, "getSize", {target}).strictEquals(target), encode(call(target, "toArray"))});
    }
    const auto pixels = NativeArray_detail::wrap(InterpolantArray(Uint8Array{1, 2, 3, 4}));
    auto texture = nativeTextureConstructor().construct({object({{"data", pixels}, {"width", 2}, {"height", 1}})});
    texture.set("uuid", "texture"); texture.get("source").set("uuid", "source");
    texture.set("userData", object({{"nested", object({{"v", 2}})}}));
    row("texture serialize", encode(call(texture, "toJSON"))); auto meta = object({{"textures", object()}, {"images", object()}});
    const auto serial = call(texture, "toJSON", {meta}); row("texture serialization cache", A{serial.strictEquals(call(texture, "toJSON", {meta})), encode(meta)});
    for (const auto& value : Args{false, 1, "true", true}) { texture.set("needsUpdate", value); texture.set("needsPMREMUpdate", value); row("texture update flags", A{encode(texture.get("version")), encode(texture.get("source").get("version")), encode(texture.get("pmremVersion"))}); }
    call(texture, "addUpdateRange", {1, 2}); call(texture, "addUpdateRange", {3, 4}); row("texture update ranges", encode(texture.get("updateRanges"))); call(texture, "clearUpdateRanges"); row("texture clear update ranges", encode(texture.get("updateRanges")));
    for (const auto& mapping : Args{UVMapping, 999}) for (const auto& wrap : Args{RepeatWrapping, ClampToEdgeWrapping, MirroredRepeatWrapping}) {
        auto uv = nativeVector2Constructor().construct({-1.25, 2.25}); texture.set("mapping", mapping); texture.set("wrapS", wrap); texture.set("wrapT", wrap); const auto value = call(texture, "transformUv", {uv}); row("texture transformUv", A{value.strictEquals(uv), encode(call(uv, "toArray"))});
    }
}
void garbageCollection() {
    auto keeper = JSValue::object(); const auto ctor = nativeRenderTargetConstructor();
    { auto target = ctor.construct({2, 3, object({{"count", 2}})}); }
    keeper.collectGarbage(); JSWeakValue targetWeak, textureWeak, sourceWeak, vectorWeak;
    {
        auto target = ctor.construct({2, 3, object({{"count", 2}})}); const auto texture = target.get("texture");
        targetWeak = JSWeakValue(target); textureWeak = JSWeakValue(texture); sourceWeak = JSWeakValue(texture.get("source")); vectorWeak = JSWeakValue(target.get("viewport"));
        call(target, "addEventListener", {"dispose", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }, {target})});
    }
    keeper.collectGarbage();
    row("managed resource cycle collection", A{!targetWeak.lock().has_value(), !textureWeak.lock().has_value(), !sourceWeak.lock().has_value(), !vectorWeak.lock().has_value()});
}
void intrinsics() {
    const auto objectCtor = JSValue::intrinsicObjectConstructor(), arrayCtor = JSValue::intrinsicArrayConstructor();
    for (const auto& value : Args{JSValue{}, nullptr, false, 4, "a😀", array({1, 2}), object({{"x", 1}}), NativeArray_detail::wrap(InterpolantArray(Uint8Array{1, 2}))}) {
        row("Object.keys", attempt([&] { return objectCtor.get("keys").call(objectCtor, {value}); }));
        row("Array.isArray", encode(arrayCtor.get("isArray").call({}, {value})));
    }
    auto spoof = JSValue::object(arrayCtor.get("prototype")); auto actual = JSValue::array(); actual.setPrototype(objectCtor.get("prototype"));
    row("Array.isArray internal brand", A{encode(arrayCtor.get("isArray").call({}, {spoof})), encode(arrayCtor.get("isArray").call({}, {actual}))});
    for (unsigned mode = 0; mode < 5; ++mode) {
        auto trace = JSValue::array(), data = object({{"a", 1}, {"b", 2}, {"c", 3}}); JSPropertyDescriptor getter; getter.enumerable = getter.configurable = true;
        getter.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSTypeError("Object.keys must not read getter"); }); data.defineProperty("b", getter);
        const auto payload = std::make_shared<unsigned>(mode); auto proxy = JSValue::hostObject(payload, payload.get(), "test.ObjectKeys"); proxy.setHostSlot("trace", trace); proxy.setHostSlot("data", data);
        static const auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>();
            h->ownPropertyNames = [](const JSValue& self) -> std::vector<std::string> { call(self.getHostSlot("trace"), "push", {"ownKeys"}); return {"a", "b", "c"}; };
            h->getOwnProperty = [](const JSValue& self, const std::string& key) -> JSHostPropertyResult {
                call(self.getHostSlot("trace"), "push", {array({"descriptor", key})}); auto data = self.getHostSlot("data");
                const auto mode = *std::static_pointer_cast<unsigned>(self.hostPayload("test.ObjectKeys"));
                if (key == "a") { if (mode == 1) data.erase("b"); if (mode == 2) { JSPropertyDescriptor d; d.enumerable = false; data.defineProperty("c", d); } if (mode == 3) data.set("d", 4); if (mode == 4) throw JSTypeError("descriptor"); }
                return {true, data.getOwnPropertyDescriptor(key)};
            }; return h;
        }(); proxy.setHostHooks(hooks);
        row("Object.keys descriptor protocol", A{attempt([&] { return objectCtor.get("keys").call(objectCtor, {proxy}); }), encode(trace)});
    }
}
void evaluationOrder() {
    for (unsigned mode = 0; mode < 6; ++mode) {
        auto texture = nativeTextureConstructor().construct(), uv = nativeVector2Constructor().construct({0.25, 0.5}), trace = JSValue::array(); texture.set("flipY", false);
        JSPropertyDescriptor method; method.configurable = true;
        method.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            call(c[0], "push", {"get applyMatrix3"}); if (c[1].number() == 2) throw JSTypeError("method"); if (c[1].number() == 5) return 4;
            if (c[1].number() == 4) { auto texture = c[3]; JSPropertyDescriptor d; d.configurable = true; d.value = object({{"tag", "changed matrix"}}); texture.defineProperty("matrix", d); }
            return JSValue::function([](const JSValue& self, const Args& a, const Args& c) -> JSValue { call(c[0], "push", {array({"call original", self.strictEquals(c[1]), a[0]})}); return self; }, {c[0], c[2]});
        }, {trace, mode, uv, texture}); uv.defineProperty("applyMatrix3", method);
        JSPropertyDescriptor matrix; matrix.configurable = true;
        matrix.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            call(c[0], "push", {"get matrix"}); if (c[1].number() == 3) throw JSTypeError("matrix");
            if (c[1].number() == 1) { auto uv = c[2]; JSPropertyDescriptor replacement; replacement.configurable = true; replacement.value = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"call replacement"}); return {}; }, {c[0]}); uv.defineProperty("applyMatrix3", replacement); }
            return object({{"tag", "matrix"}});
        }, {trace, mode, uv}); texture.defineProperty("matrix", matrix);
        row("Texture.transformUv evaluation order", A{attempt([&] { return call(texture, "transformUv", {uv}).strictEquals(uv); }), encode(trace)});
    }
    for (unsigned mode = 0; mode < 6; ++mode) {
        auto trace = JSValue::array(), image = object(), entry = object(), prototype = JSValue::intrinsicArrayConstructor().get("prototype");
        const auto saved = *prototype.getOwnPropertyDescriptor("push"); const auto original = prototype.get("push"), pixels = NativeArray_detail::wrap(InterpolantArray(Uint8Array{1, 2}));
        JSPropertyDescriptor data; data.configurable = true;
        data.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            c[0].call(c[1], {"get data"}); if (c[2].number() == 3) throw JSTypeError("data");
            if (c[2].number() == 4) { auto prototype = c[4]; JSPropertyDescriptor d; d.configurable = true; d.value = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { c[0].call(c[1], {"call replacement"}); return 99; }, {c[0], c[1]}); prototype.defineProperty("push", d); }
            return c[3];
        }, {original, trace, mode, pixels, prototype}); image.defineProperty("data", data); image.set("width", 2); image.set("height", 1);
        JSPropertyDescriptor flag; flag.configurable = true;
        flag.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { c[0].call(c[1], {"get isDataTexture"}); if (c[2].number() == 5) throw JSTypeError("flag"); return c[2].number() == 1; }, {original, trace, mode});
        auto item = mode == 1 ? entry : image; item.defineProperty("isDataTexture", flag);
        if (mode == 1) { JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { c[0].call(c[1], {"get image"}); return c[2]; }, {original, trace, image}); entry.defineProperty("image", d); }
        auto source = nativeTextureSourceConstructor().construct({array({item})}); source.set("uuid", "source");
        JSPropertyDescriptor push; push.configurable = true;
        push.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            c[0].call(c[1], {"get push"}); if (c[2].number() == 2) throw JSTypeError("push");
            return JSValue::function([](const JSValue& self, const Args& a, const Args& c) -> JSValue { c[0].call(c[1], {"call original"}); return c[0].call(self, a); }, {c[0], c[1]});
        }, {original, trace, mode}); prototype.defineProperty("push", push);
        const auto result = attempt([&] { return call(source, "toJSON"); }); prototype.defineProperty("push", saved);
        row("TextureSource.toJSON evaluation order", A{result, encode(trace)});
    }
    for (unsigned count = 2; count <= 4; ++count) {
        const auto ctor = count == 2 ? nativeVector2Constructor() : count == 3 ? nativeVector3Constructor() : nativeVector4Constructor();
        for (const auto* method : {"clone", "equals"}) {
            auto self = ctor.construct(), other = ctor.construct(), trace = JSValue::array();
            for (const auto& value : {self, other}) for (unsigned i = 0; i < count; ++i) {
                const std::string key(1, "xyzw"[i]); auto target = value; JSPropertyDescriptor d; d.configurable = true;
                d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {array({c[1], c[2]})}); return c[3]; }, {trace, value.strictEquals(self) ? "self" : "other", key, i + 1}); target.defineProperty(key, d);
            }
            JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"get constructor"}); return c[1]; }, {trace, ctor}); self.defineProperty("constructor", d);
            const auto result = attempt([&] { return std::string(method) == "clone" ? call(call(self, method), "toArray") : call(self, method, {other}); });
            row("Vector evaluation order", A{count, method, result, encode(trace)});
        }
    }
}
}
int main() { try { metadata(); construction(); sizeCases(); depthCases(); copyCases(); cubeCases(); protocols(); garbageCollection(); intrinsics(); evaluationOrder(); } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }

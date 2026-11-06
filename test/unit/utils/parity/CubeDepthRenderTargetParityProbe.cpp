#include "textures/CubeDepthTexture.h"
#include "textures/DepthTexture.h"
#include "renderers/WebGLCubeRenderTarget.h"
#include "renderers/WebGLRenderTarget.h"
#include "core/RenderTarget.h"
#include "core/JSPromise.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <tuple>

namespace {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
JSValue call(JSValue self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
JSValue array(const Args& values = {}) { auto result = JSValue::array(); for (const auto& value : values) call(result, "push", {value}); return result; }
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> values = {}) { auto result = JSValue::object(); for (const auto& [key, value] : values) result.set(key, value); return result; }
void push(JSValue trace, const Args& values) { call(trace, "push", {array(values)}); }
JSValue arg(const Args& args, unsigned i) { return i < args.size() ? args[i] : JSValue{}; }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "#undefined"; if (value.isNull()) return nullptr;
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Number) { const auto n = value.number(); if (std::isnan(n)) return "#NaN"; if (std::isinf(n)) return n < 0 ? "#-Infinity" : "#Infinity"; if (n == 0 && std::signbit(n)) return "#-0"; return n; }
    if (value.isCallable()) return "#function";
    if (value.hasOwn("tag")) return O{{"ref", encode(value.get("tag"))}};
    if (value.type() == JSValue::Type::Array) { A result; for (unsigned i = 0; i < value.length(); ++i) result.push_back(encode(value.get(JSValue(i)))); return result; }
    O result; for (const auto& key : value.keys()) result[key] = encode(value.get(key)); return result;
}
void row(const char* kind, JsonValue value) { std::cout << JsonValue(O{{"kind", kind}, {"value", std::move(value)}}).dump() << '\n'; }
JsonValue names(const JSValue& value, bool omitResourceId = false) { A result; for (const auto& key : value.ownPropertyNames()) if (!omitResourceId || (key != "id" && key != "uuid")) result.emplace_back(key); return result; }
template<class F> JsonValue attempt(F&& fn) { try { return encode(fn()); } catch (...) { return encode(nativeExceptionValue(std::current_exception()).get("name")); } }
JsonValue textureState(JSValue texture, JSValue target = nullptr) {
    const auto image = texture.get("image"); const bool cube = image.type() == JSValue::Type::Array || (image.isObject() && image.hasOwn("0"));
    A options; for (const auto* key : {"mapping", "wrapS", "wrapT", "magFilter", "minFilter", "format", "type", "anisotropy", "colorSpace", "flipY", "generateMipmaps", "compareFunction", "isDepthTexture", "isCubeTexture", "isCubeDepthTexture", "isRenderTargetTexture", "version"}) options.push_back(encode(texture.get(key)));
    return O{{"own", names(texture, true)}, {"options", options}, {"image", encode(image)}, {"imageArray", image.type() == JSValue::Type::Array},
        {"aliases", cube && image.get("0").strictEquals(image.get("5"))}, {"imagesAlias", texture.get("images").strictEquals(image)},
        {"sourceVersion", encode(texture.get("source").get("version"))}, {"owner", texture.get("renderTarget").strictEquals(target)}};
}
JsonValue targetState(JSValue target) {
    A textures; auto values = target.get("textures"); for (unsigned i = 0; i < values.length(); ++i) textures.push_back(textureState(values.get(JSValue(i)), target));
    A flags; for (const auto* key : {"isWebGLRenderTarget", "isWebGLCubeRenderTarget", "depthBuffer", "stencilBuffer", "samples", "scissorTest"}) flags.push_back(encode(target.get(key)));
    return O{{"own", names(target)}, {"size", A{encode(target.get("width")), encode(target.get("height")), encode(target.get("depth"))}},
        {"flags", flags}, {"viewport", encode(call(target.get("viewport"), "toArray"))}, {"textures", textures}};
}
void metadata() {
    for (const auto& ctor : {nativeCubeDepthTextureConstructor(), nativeWebGLCubeRenderTargetConstructor()}) {
        auto prototype = ctor.get("prototype"); A entries;
        for (const auto& key : prototype.ownPropertyNames()) {
            const auto d = *prototype.getOwnPropertyDescriptor(key); const auto fn = d.value.value_or(d.getter.value_or(JSValue{})), setter = d.setter.value_or(JSValue{});
            entries.emplace_back(A{key, d.writable ? JsonValue(*d.writable) : JsonValue(nullptr), d.enumerable.value_or(false), d.configurable.value_or(false),
                encode(fn.get("name")), encode(fn.get("length")), setter.isCallable() ? JsonValue(A{encode(setter.get("name")), encode(setter.get("length"))}) : JsonValue(nullptr),
                encode(call(fn, "toString")), setter.isCallable() ? encode(call(setter, "toString")) : JsonValue(nullptr)});
        }
        row("metadata", A{encode(ctor.get("name")), encode(ctor.get("length")), encode(ctor.getPrototype().get("name")), prototype.getPrototype().strictEquals(ctor.getPrototype().get("prototype")), entries, attempt([&] { return ctor.call({}); })});
    }
    row("module identity", A{nativeCubeDepthTextureConstructor().strictEquals(nativeCubeDepthTextureModule().get("CubeDepthTexture")), nativeWebGLCubeRenderTargetConstructor().strictEquals(nativeWebGLCubeRenderTargetModule().get("WebGLCubeRenderTarget")), nativeCubeDepthTextureConstructor().getPrototype().strictEquals(nativeDepthTextureConstructor())});
}
void construction() {
    const auto depth = nativeCubeDepthTextureConstructor(), cube = nativeWebGLCubeRenderTargetConstructor();
    const std::vector<Args> cases{{}, {3}, {nullptr}, {-0.0}, {"4"}, {object({{"tag", "size"}})}, {std::nan("")},
        {5, {}, {}, {}, {}, {}, {}, {}, {}}, {2, 1015, 305, 1000, 1002, 1006, 1008, 3, 1026},
        {2, nullptr, nullptr, false, "wrap", nullptr, true, "anisotropy", 1026}, {2, 1014, 301, 1001, 1001, 1003, 1003, 1, 1027}};
    for (const auto& args : cases) row("depth construction", textureState(depth.construct(args)));
    row("depth invalid format", attempt([&] { return depth.construct({2, {}, {}, {}, {}, {}, {}, {}, 999}); }));
    for (const auto& size : Args{{}, nullptr, -0.0, -2.5, "4", object({{"tag", "size"}}), std::nan("")}) for (int count : {0, 1, 2})
        row("cube target construction", targetState(cube.construct({size, object({{"count", count}, {"depth", 3}, {"depthBuffer", false}, {"samples", 2}, {"minFilter", 1003}, {"wrapS", 1000}, {"colorSpace", "srgb"}})})));
    for (const auto& options : Args{{}, nullptr, false, 0, "x"}) row("cube primitive options", attempt([&] { return JSValue::fromJSON(targetState(cube.construct({3, options}))); }));
}
void aliasesAndCopies() {
    const auto depth = nativeCubeDepthTextureConstructor(), cube = nativeWebGLCubeRenderTargetConstructor();
    const auto descriptor = *depth.get("prototype").getOwnPropertyDescriptor("images");
    for (const auto& value : Args{{}, nullptr, false, 3, object({{"tag", "image"}}), array({object({{"width", 4}})})}) {
        auto self = object({{"image", object({{"tag", "old"}})}});
        const auto result = attempt([&] { return descriptor.setter->call(self, {value}); });
        row("images accessor", A{result, encode(self.get("image")), descriptor.getter->call(self).strictEquals(value)});
    }
    for (int mode = 0; mode < 4; ++mode) {
        auto source = depth.construct({7}); source.set("compareFunction", 515); source.set("name", "source");
        auto result = mode % 2 ? call(depth.construct({2}), "copy", {source}) : call(source, "clone");
        const auto originalImage = source.get("image"); result.get("image").get("0").set("width", 19);
        row("depth structural copy", A{mode, textureState(result), !result.strictEquals(source), !result.get("source").strictEquals(source.get("source")), !result.get("image").strictEquals(source.get("image")),
            result.get("image").get("0").strictEquals(source.get("image").get("0")), source.get("image").strictEquals(originalImage), encode(source.get("image").get("5").get("width")), result.getPrototype().strictEquals(depth.get("prototype"))});
    }
    for (int mode = 0; mode < 4; ++mode) {
        auto source = cube.construct({4, object({{"count", 2}})}); source.set("depthTexture", depth.construct({4}));
        auto result = mode % 2 ? call(cube.construct({2}), "copy", {source}) : call(source, "clone");
        row("cube structural copy", A{mode, targetState(result), !result.strictEquals(source), !result.get("texture").get("source").strictEquals(source.get("texture").get("source")),
            !result.get("texture").get("image").strictEquals(source.get("texture").get("image")), result.get("texture").get("image").get("0").strictEquals(source.get("texture").get("image").get("0")),
            !result.get("depthTexture").strictEquals(source.get("depthTexture")), !result.get("depthTexture").get("source").strictEquals(source.get("depthTexture").get("source")),
            result.get("depthTexture").get("renderTarget").strictEquals(result), result.getPrototype().strictEquals(cube.get("prototype"))});
    }
    for (auto source : {depth.construct(), cube.construct()}) {
        auto trace = JSValue::array(), state = object({{"trace", trace}, {"source", source}});
        auto result = object({{"copy", JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue { call(c[0], "push", {arg(args, 0).strictEquals(c[1])}); return "copied"; }, {trace, source})}});
        auto replacement = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& c) -> JSValue { push(c[0].get("trace"), {"construct", double(args.size()), target.strictEquals(c[0].get("replacement"))}); return c[0].get("result"); }, {state}, "Replacement", 0);
        state.set("result", result); state.set("replacement", replacement);
        JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"constructor"}); return c[1]; }, {trace, replacement}); source.defineProperty("constructor", d);
        row("dynamic clone", A{attempt([&] { return call(source, "clone"); }), encode(trace)});
    }
}
void dynamicSuper() {
    for (bool cube : {false, true}) for (int mode = 0; mode < 4; ++mode) {
        auto ctor = cube ? nativeWebGLCubeRenderTargetConstructor() : nativeCubeDepthTextureConstructor(); const auto previous = ctor.getPrototype();
        // Pinned fixture uses function Sub(), whose own prototype is writable.
        auto trace = JSValue::array(); auto sub = JSValue::constructor([](const JSValue& self, const Args&, const Args&) { return self; }, {}, "Sub", 0, JSValue::ConstructorKind::Callable);
        auto prototype = JSValue::object(); prototype.setPrototype(ctor.get("prototype")); sub.set("prototype", prototype);
        auto state = object({{"trace", trace}, {"sub", sub}, {"cube", cube}, {"mode", mode}});
        auto base = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& c) -> JSValue {
            auto state = c[0], trace = state.get("trace"); const int mode = int(state.get("mode").number()); const bool cube = state.get("cube").toBoolean();
            push(trace, {"super", target.strictEquals(state.get("sub")), array(args)}); if (mode == 1) throw JSTypeError("super");
            auto result = JSValue::object(); result.setPrototype(target.get("prototype"));
            if (cube) {
                result.set("textures", JSValue::array());
                result.set("_setTextureOptions", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { push(c[0], {"options", self.strictEquals(c[1]), arg(args, 0), self.get("texture").get("isCubeTexture")}); if (c[2].number() == 2) throw JSTypeError("options"); return {}; }, {trace, result, mode}));
            } else if (mode == 2) {
                JSPropertyDescriptor d; d.setter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"set image"}); throw JSTypeError("image"); }, {trace}); result.defineProperty("image", d);
            }
            if (mode == 3) { JSPropertyDescriptor d; d.setter = JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue { push(c[0], {"flag", arg(args, 0)}); throw JSTypeError("flag"); }, {trace}); result.defineProperty(cube ? "isWebGLCubeRenderTarget" : "isCubeDepthTexture", d); }
            return result;
        }, {state}, "Base", 0);
        ctor.setPrototype(base);
        const auto result = attempt([&] { auto value = ctor.construct(cube ? Args{6, object({{"tag", "options"}})} : Args{6}, sub); return array({value.getPrototype().strictEquals(sub.get("prototype")), value.get("isCubeDepthTexture"), value.get("isCubeTexture"), value.get("isWebGLCubeRenderTarget"), cube ? value.get("texture").get("image") : value.get("image")}); });
        ctor.setPrototype(previous); row("dynamic super", A{cube, mode, result, encode(trace)});
    }
}
void optionsOrder() {
    auto prototype = nativeRenderTargetConstructor().get("prototype"); const auto original = prototype.get("_setTextureOptions");
    for (int mode = 0; mode < 4; ++mode) {
        auto trace = JSValue::array(), options = JSValue::object(), counter = object({{"reads", 0}});
        JSPropertyDescriptor d; d.enumerable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { auto counter = c[1]; const int n = int(counter.get("reads").number()) + 1; counter.set("reads", n); push(c[0], {"minFilter", n}); return n % 2 ? 1003 : 1006; }, {trace, counter}); options.defineProperty("minFilter", d);
        prototype.set("_setTextureOptions", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            const auto texture = self.get("texture"); push(c[0], {"options", arg(args, 0).strictEquals(c[1]), self.get("isWebGLRenderTarget").strictEquals(true), texture.get("isCubeTexture").strictEquals(true), texture.get("renderTarget").strictEquals(self)});
            if (c[2].number() == 1 && texture.get("isCubeTexture").toBoolean()) throw JSTypeError("second"); if (c[2].number() == 2) return "ignored"; return c[3].call(self, {arg(args, 0)});
        }, {trace, options, mode, original}));
        const auto result = attempt([&] { return JSValue::fromJSON(targetState(nativeWebGLCubeRenderTargetConstructor().construct({3, options}))); });
        prototype.set("_setTextureOptions", original); row("options source order", A{mode, result, encode(trace)});
    }
}
void clearCases() {
    for (int mode = 0; mode < 12; ++mode) {
        auto target = nativeWebGLCubeRenderTargetConstructor().construct(), previous = object({{"tag", "previous"}}), trace = JSValue::array(), renderer = JSValue::object();
        auto state = object({{"target", target}, {"previous", previous}, {"trace", trace}, {"renderer", renderer}, {"mode", mode}, {"face", -1}, {"nested", false}});
        renderer.set("getRenderTarget", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue { auto state = c[0]; push(state.get("trace"), {"get", self.strictEquals(state.get("renderer"))}); if (state.get("mode").number() == 3) throw JSTypeError("get"); return state.get("previous"); }, {state}));
        renderer.set("setRenderTarget", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { auto state = c[0]; auto next = arg(args, 1); state.set("face", next); push(state.get("trace"), {"set", self.strictEquals(state.get("renderer")), arg(args, 0).strictEquals(state.get("target")) ? JSValue("target") : arg(args, 0), next, double(args.size())}); if (state.get("mode").number() == 1 && next.strictEquals(2)) throw JSTypeError("set"); return {}; }, {state}));
        auto clear = JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { auto state = c[0]; push(state.get("trace"), {"clear", self.strictEquals(state.get("renderer")), array(args)}); const int mode = int(state.get("mode").number()); if (mode == 2 && state.get("face").strictEquals(2)) throw JSTypeError("clear"); if (mode == 10 && !state.get("nested").toBoolean()) { state.set("nested", true); call(state.get("target"), "clear", {state.get("renderer"), false, true, false}); } return {}; }, {state});
        if (mode == 11) { JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"get clear"}); return c[1]; }, {trace, clear}); renderer.defineProperty("clear", d); } else renderer.set("clear", clear);
        const Args args = mode < 4 || mode >= 10 ? Args{renderer} : Args{renderer, mode == 4 ? JSValue{} : mode == 5 ? JSValue(nullptr) : mode == 6 ? JSValue(false) : mode == 7 ? JSValue(0) : object({{"tag", "color"}}), false, 0};
        row("clear", A{mode, attempt([&] { return call(target, "clear", args); }), encode(trace)});
    }
}
void equirectCases() {
    for (int mode = 0; mode < 10; ++mode) {
        auto target = nativeWebGLCubeRenderTargetConstructor().construct({8}), texture = nativeTextureConstructor().construct(), previous = object({{"tag", "previous"}}), trace = JSValue::array();
        auto values = object({{"type", 1015}, {"colorSpace", "srgb"}, {"generateMipmaps", true}, {"minFilter", mode == 6 ? 1003 : 1008}, {"magFilter", 1006}});
        for (const auto* key : {"type", "colorSpace", "generateMipmaps", "minFilter", "magFilter"}) {
            JSPropertyDescriptor d; d.enumerable = d.configurable = true;
            d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], {"get texture", c[2]}); return c[1].get(c[2]); }, {trace, values, key});
            d.setter = JSValue::function([](const JSValue&, const Args& args, const Args& c) -> JSValue { push(c[0], {"set texture", c[2], arg(args, 0)}); auto values = c[1]; values.set(c[2], arg(args, 0)); return {}; }, {trace, values, key}); texture.defineProperty(key, d);
        }
        auto renderer = object({{"coordinateSystem", mode == 8 ? 2001 : 2000}, {"xr", object({{"enabled", true}})}, {"autoClear", false}, {"isWebGLRenderer", mode != 9}, {"reversedDepthBuffer", mode == 9}});
        auto state = object({{"target", target}, {"texture", texture}, {"previous", previous}, {"trace", trace}, {"values", values}, {"renderer", renderer}, {"current", previous}, {"active", 3}, {"mip", 2}, {"draws", 0}, {"mode", mode}});
        auto reverse = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"reversed"}); return c[1].number() == 7; }, {trace, mode});
        renderer.set("state", object({{"buffers", object({{"depth", object({{"getReversed", reverse}})}})}}));
        for (const auto& [method, field, label] : std::initializer_list<std::tuple<const char*, const char*, const char*>>{{"getRenderTarget", "current", "get target"}, {"getActiveCubeFace", "active", "get face"}, {"getActiveMipmapLevel", "mip", "get mip"}})
            renderer.set(method, JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0].get("trace"), "push", {c[2]}); return c[0].get(c[1]); }, {state, field, label}));
        renderer.set("setRenderTarget", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { auto state = c[0]; state.set("current", arg(args, 0)); state.set("active", arg(args, 1)); state.set("mip", arg(args, 2)); push(state.get("trace"), {"set", self.strictEquals(state.get("renderer")), arg(args, 0).strictEquals(state.get("target")) ? JSValue("target") : arg(args, 0), arg(args, 1), arg(args, 2)}); return {}; }, {state}));
        renderer.set("clear", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { push(c[0], {"clear", self.strictEquals(c[1]), array(args)}); return {}; }, {trace, renderer}));
        renderer.set("clearDepth", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {"clearDepth"}); return {}; }, {trace}));
        renderer.set("render", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            auto state = c[0], mesh = arg(args, 0), camera = arg(args, 1), trace = state.get("trace"), texture = state.get("texture"), target = state.get("target"), renderer = state.get("renderer"); const int mode = int(state.get("mode").number());
            push(trace, {"render", self.strictEquals(renderer), mesh.get("isMesh"), camera.get("isPerspectiveCamera"), state.get("active"), state.get("mip"), texture.get("minFilter"), target.get("texture").get("mipmapsAutoUpdate"), renderer.get("xr").get("enabled"), camera.get("near"), camera.get("far"), call(camera.get("up"), "toArray")});
            const int draws = int(state.get("draws").number()); state.set("draws", draws + 1);
            if (draws == 0) {
                state.set("meshSeen", mesh); auto geometry = mesh.get("geometry"), material = mesh.get("material");
                push(trace, {"payload", geometry.get("parameters"), geometry.get("attributes").get("position").get("count"), geometry.get("index").get("count"), material.get("name"), material.get("side"), material.get("blending"), material.get("uniforms").get("tEquirect").get("value").strictEquals(texture), material.get("vertexShader"), material.get("fragmentShader")});
                call(geometry, "addEventListener", {"dispose", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { push(c[0].get("trace"), {"dispose geometry", arg(args, 0).get("target").strictEquals(self), c[0].get("values").get("minFilter"), c[0].get("renderer").get("xr").get("enabled")}); if (c[0].get("mode").number() == 5) throw JSTypeError("dispose"); return {}; }, {state})});
                call(material, "addEventListener", {"dispose", JSValue::function([](const JSValue& self, const Args& args, const Args& c) -> JSValue { push(c[0].get("trace"), {"dispose material", arg(args, 0).get("target").strictEquals(self), c[0].get("values").get("minFilter"), c[0].get("renderer").get("xr").get("enabled")}); return {}; }, {state})});
            } else if (mode != 4) push(trace, {"same mesh", mesh.strictEquals(state.get("meshSeen"))});
            if (mode == 1 && state.get("active").strictEquals(2)) throw JSTypeError("render");
            if (mode == 2 && state.get("active").strictEquals(0)) texture.set("minFilter", 1003);
            if (mode == 3 && state.get("active").strictEquals(5)) { auto replacement = object({{"dispose", JSValue::function([](const JSValue& self, const Args&, const Args& c) -> JSValue { push(c[0], {"replacement dispose", self.strictEquals(c[1].get("geometry"))}); return {}; }, {trace, mesh})}}); mesh.set("geometry", replacement); }
            if (mode == 4 && state.get("active").strictEquals(0)) push(trace, {"nested clear result", call(target, "clear", {renderer, false, false, true})});
            return {};
        }, {state}));
        const auto result = attempt([&] { return JSValue(call(target, "fromEquirectangularTexture", {renderer, texture}).strictEquals(target)); });
        auto outputTexture = target.get("texture");
        row("equirect source", A{mode, result, encode(trace), encode(values), state.get("current").strictEquals(previous), encode(state.get("active")), encode(state.get("mip")), encode(renderer.get("xr").get("enabled")),
            encode(outputTexture.get("mipmapsAutoUpdate")), encode(outputTexture.get("needsPMREMUpdate")), A{encode(outputTexture.get("type")), encode(outputTexture.get("colorSpace")), encode(outputTexture.get("generateMipmaps")), encode(outputTexture.get("minFilter")), encode(outputTexture.get("magFilter"))}});
    }
}
}
int main() { try { metadata(); construction(); aliasesAndCopies(); dynamicSuper(); optionsOrder(); clearCases(); equirectCases(); return 0; } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; } }

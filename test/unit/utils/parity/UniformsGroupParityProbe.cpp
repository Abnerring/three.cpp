#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/NativeArray.h"
#include "renderers/common/UniformsGroup.h"
#include "renderers/common/nodes/NodeUniformsGroup.h"
#include "renderers/common/nodes/NodeUniform.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
JSValue array(const Args& values = {}) { auto out = JSValue::array(); for (const auto& v : values) call(out, "push", {v}); return out; }
void getter(JSValue self, const std::string& name, const JSValue& function) { JSPropertyDescriptor d; d.getter = function; d.enumerable = d.configurable = true; self.defineProperty(name, d); }
JSValue normal(const JSValue& value, const JSValue& state, unsigned depth = 0) {
    if (value.isUndefined()) return "#undefined"; if (value.isCallable()) return "#function"; if (value.isSymbol()) return "#symbol";
    if (value.type() == JSValue::Type::Number) {
        if (std::isnan(value.number())) return "#NaN"; if (std::isinf(value.number())) return value.number() > 0 ? "#Infinity" : "#-Infinity";
        if (value.number() == 0 && std::signbit(value.number())) return "#-0";
    }
    if (!value.isObject()) return value;
    for (const auto* key : {"group", "uniform", "value", "values", "buffer"}) if (value.strictEquals(state.get(key))) return std::string("#") + key;
    if (isNativeMap(value)) return array(); if (isNativeSet(value)) return array(); if (depth > 8) return "#depth";
    auto out = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    for (const auto& key : value.keys()) out.set(key, normal(value.get(key), state, depth + 1)); return out;
}
JSValue decode(const JSValue& value) {
    if (!value.isObject()) return value;
    if (value.get("tag").strictEquals("undefined")) return {};
    if (value.get("tag").strictEquals("nan")) return std::numeric_limits<double>::quiet_NaN();
    if (value.get("tag").strictEquals("negative-zero")) return -0.0;
    if (value.get("tag").strictEquals("infinity")) return std::numeric_limits<double>::infinity();
    auto out = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    for (const auto& key : value.keys()) out.set(key, decode(value.get(key))); return out;
}
void event(const JSValue& state, const Args& values) { auto row = array(); for (const auto& v : values) call(row, "push", {normal(v, state)}); call(state.get("events"), "push", {row}); }
JSValue list(const JSValue& iterable, const JSValue& state) { auto out = array(); auto iterator = getJSIterator(iterable); while (const auto item = stepJSIterator(iterator)) call(out, "push", {normal(*item, state)}); return out; }
JSValue makeUniform(const JSValue& spec) {
    auto node = JSValue::object(); node.set("name", "fixture"); node.set("type", decode(spec.get("type"))); node.set("value", decode(spec.get("value")));
    return nativeNodeUniforms().get(spec.get("kind").string() + "NodeUniform").construct({node});
}
JSValue snapshot(const JSValue& group, const JSValue& state) {
    auto out = JSValue::object(); out.set("values", list(group.get("values"), state));
    out.set("bytes", list(NativeArray_detail::constructor(1).construct({group.get("buffer").get("buffer")}), state));
    out.set("ranges", normal(group.get("updateRanges"), state)); out.set("added", list(group.get("_addedIndices"), state));
    out.set("cacheSize", group.get("_updateRangeCache").get("size")); auto offsets = array();
    const auto uniforms = group.get("uniforms"); for (unsigned i = 0; i < uniforms.length(); ++i) {
        const auto uniform = uniforms.get(std::to_string(i)); call(offsets, "push", {array({normal(uniform.get("offset"), state), uniform.get("index")})});
    }
    out.set("offsets", offsets); return out;
}
JSValue run(const JSValue& request) {
    auto state = JSValue::object(); state.set("events", array()); auto result = JSValue::object();
    const auto ctor = request.get("nodeGroup").toBoolean() ? nativeNodeUniformsGroupConstructor() : nativeUniformsGroupConstructor();
    auto groupNode = JSValue::object(); groupNode.set("name", "fixture-node"); auto group = ctor.construct({"fixture", groupNode}); state.set("group", group);
    const auto specs = request.get("uniforms"); if (specs.type() == JSValue::Type::Array) for (unsigned i = 0; i < specs.length(); ++i) call(group, "addUniform", {makeUniform(specs.get(std::to_string(i)))});
    const auto kind = request.get("kind").string(); const auto proto = nativeUniformsGroupConstructor().get("prototype"), oldParent = proto.getPrototype(); const auto oldCtorParent = ctor.getPrototype();
    ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&, const Args& args, const Args& c)->JSValue { event(c[0], {"console", arg(args, 0), arg(args, 1).get("startsWith").call(arg(args, 1), {"THREE.WebGPUUniformsGroup:"})}); return {}; }, {state}));
    try {
        if (kind == "metadata") {
            auto fields = JSValue::object(); for (const auto& key : group.ownPropertyNames()) fields.set(key, key == "id" ? JSValue("#id") : normal(group.get(key), state)); result.set("fields", fields);
            auto methods = array(); const auto prototype = ctor.get("prototype"); for (const auto& key : prototype.ownPropertyNames()) {
                if (key == "constructor") continue; const auto d = *prototype.getOwnPropertyDescriptor(key); const auto fn = d.getter ? *d.getter : prototype.get(key);
                call(methods, "push", {array({key, fn.get("name"), fn.get("length"), fn.isConstructible(), d.enumerable.value_or(false), d.configurable.value_or(false), d.writable ? JSValue(*d.writable) : JSValue("#accessor")})});
            }
            result.set("methods", methods); result.set("arity", ctor.get("length")); result.set("groupNode", group.get("groupNode").strictEquals(groupNode));
            if (request.get("nodeGroup").toBoolean()) result.set("idDelta", ctor.construct().get("id").number() - group.get("id").number());
        } else if (kind == "layout") {
            result.set("initial", array({group.get("_buffer").isNull(), group.get("_values").isNull()})); result.set("byteLength", group.get("byteLength"));
            const auto buffer = group.get("buffer"), values = group.get("values"); result.set("identity", array({buffer.strictEquals(group.get("buffer")), values.strictEquals(group.get("values"))})); result.set("snapshot", snapshot(group, state));
        } else if (kind == "updates") {
            auto rows = array(); const auto steps = request.get("steps");
            for (unsigned i = 0; i < steps.length(); ++i) {
                const auto step = steps.get(std::to_string(i)); if (step.get("clear").toBoolean()) call(group, "clearUpdateRanges");
                const auto replacements = step.get("values"); if (replacements.type() == JSValue::Type::Array) for (unsigned j = 0; j < replacements.length(); ++j) { auto node = group.get("uniforms").get(std::to_string(j)).get("nodeUniform"); node.set("value", decode(replacements.get(std::to_string(j)))); }
                const auto updated = call(group, "update"); auto row = snapshot(group, state); row.set("updated", normal(updated, state)); call(rows, "push", {row});
            }
            result.set("rows", rows);
        } else if (kind == "ranges") {
            const auto uniforms = group.get("uniforms"); group.get("buffer"); const auto uniform = uniforms.get("0");
            call(group, "addUniformUpdateRange", {uniform}); const auto range = group.get("updateRanges").get("0");
            call(group, "addUniformUpdateRange", {uniform}); result.set("dedup", group.get("updateRanges").get("length"));
            call(group, "clearUpdateRanges"); auto mutableUniform = uniform; mutableUniform.set("offset", 7); mutableUniform.set("itemSize", 3); call(group, "addUniformUpdateRange", {uniform});
            result.set("identity", range.strictEquals(group.get("updateRanges").get("0"))); result.set("range", normal(range, state)); result.set("cacheSize", group.get("_updateRangeCache").get("size"));
            const auto oldBuffer = group.get("buffer"), oldValues = group.get("values"); call(group, "release"); result.set("released", array({group.get("_buffer").isNull(), group.get("_values").isNull()}));
            result.set("newIdentity", array({!oldBuffer.strictEquals(group.get("buffer")), !oldValues.strictEquals(group.get("values"))})); result.set("rangeAfterRelease", normal(group.get("updateRanges"), state));
        } else if (kind == "addRemove") {
            const auto uniform = group.get("uniforms").get("0"); result.set("add", call(group, "addUniform", {uniform}).strictEquals(group));
            result.set("remove", call(group, "removeUniform", {uniform}).strictEquals(group)); result.set("missing", call(group, "removeUniform", {JSValue::object()}).strictEquals(group)); result.set("length", group.get("uniforms").get("length"));
            result.set("lastIdentity", group.get("uniforms").get(JSValue(group.get("uniforms").length() - 1)).strictEquals(uniform)); result.set("snapshot", snapshot(group, state));
        } else if (kind == "protocol") {
            auto uniform = group.get("uniforms").get("0"); state.set("uniform", uniform); group.get("buffer"); group.get("values");
            const auto value = uniform.get("nodeUniform").get("value"); state.set("value", value); state.set("values", group.get("values")); state.set("buffer", group.get("buffer"));
            auto counts = JSValue::object(); counts.set("value", 0);
            for (const auto* name : {"getValue", "getType"}) getter(uniform, name, JSValue::function([](const JSValue& self, const Args&, const Args& c)->JSValue {
                event(c[0], {"get", c[2], self}); return JSValue::function([](const JSValue& self, const Args& args, const Args& c)->JSValue {
                    const auto state = c[0], request = c[1]; const auto name = c[2].string(); event(state, {name, self, array(args)});
                    if (name == "getValue") {
                        auto count = c[3]; const auto index = count.get("value").number(); count.set("value", index + 1);
                        if (request.get("reenter").toBoolean() && index == 0) event(state, {"reentered", call(state.get("group"), request.get("method").string().c_str(), {self})});
                        return state.get("value");
                    }
                    if (request.get("replaceValues").toBoolean()) { auto group = state.get("group"); group.set("_values", array({99, 98, 97, 96})); }
                    if (request.get("throwType").toBoolean()) throw JSTypeError("type"); return request.get("uniforms").get("0").get("type");
                }, c);
            }, {state, request, name, counts}));
            JSValue offset = request.hasOwn("offset") ? decode(request.get("offset")) : JSValue(0);
            if (request.get("offsetObject").toBoolean()) {
                offset = JSValue::object(); offset.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& c)->JSValue {
                    event(c[0], {"offset:key", arg(args, 0), c[0].get("values").get("0")}); return 0;
                }, {state}));
                auto values = state.get("values"), stored = JSValue::object(); stored.set("value", values.get("0")); JSPropertyDescriptor d;
                d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { return c[0].get("value"); }, {stored});
                d.setter = JSValue::function([](const JSValue&, const Args& args, const Args& c)->JSValue { auto stored = c[1]; stored.set("value", arg(args, 0)); event(c[0], {"values:set", arg(args, 0)}); return {}; }, {state, stored});
                d.enumerable = d.configurable = true; values.defineProperty("0", d);
            }
            getter(uniform, "offset", JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { event(c[0], {"offset"}); return c[1]; }, {state, offset}));
            if (value.isObject()) for (const auto* component : {"x", "y", "z", "w", "r", "g", "b"}) if (value.hasOwn(component)) {
                auto componentCounts = JSValue::object(); componentCounts.set("n", 0); const auto original = value.get(component);
                getter(value, component, JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { auto count = c[3]; const auto n = count.get("n").number(); count.set("n", n + 1); event(c[0], {"component", c[1], n}); return c[2]; }, {state, component, original, componentCounts}));
            }
            for (const auto* member : {"values", "buffer", "_getBufferForType", "addUniformUpdateRange"}) {
                const auto original = member == std::string("values") ? state.get("values") : member == std::string("buffer") ? state.get("buffer") : group.get(member);
                getter(group, member, JSValue::function([](const JSValue& self, const Args&, const Args& c)->JSValue { event(c[0], {"group:get", c[1], self});
                    if (!c[2].isCallable()) return c[2]; return JSValue::function([](const JSValue& self, const Args& args, const Args& c)->JSValue { event(c[0], {"group:call", c[1], self, array(args)}); return c[2].call(self, args); }, c);
                }, {state, member, original}));
            }
            result.set("updated", normal(call(group, request.get("method").string().c_str(), {uniform}), state)); result.set("snapshot", snapshot(group, JSValue::object()));
        } else if (kind == "iterator") {
            auto iterable = JSValue::object(), iterator = JSValue::object(), count = JSValue::object(); count.set("n", 0); const auto uniform = group.get("uniforms").get("0");
            iterator.set("next", JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { auto count = c[0]; const auto n = count.get("n").number(); count.set("n", n + 1); auto result = JSValue::object(); result.set("done", n >= 2); result.set("value", c[1]); return result; }, {count, uniform}));
            iterator.set("return", JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { event(c[0], {"iterator:return"}); return JSValue::object(); }, {state}));
            iterable.set(JSValue::wellKnownSymbol("iterator"), JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { return c[0]; }, {iterator})); group.set("uniforms", iterable);
            group.set("updateByType", JSValue::function([](const JSValue&, const Args&, const Args& c)->JSValue { event(c[0], {"updateByType"}); if (c[1].get("throw").toBoolean()) throw JSTypeError("update"); return decode(c[1].get("result")); }, {state, request})); result.set("updated", normal(call(group, "update"), state));
        } else if (kind == "super") {
            auto alternate = JSValue::object(oldParent); getter(alternate, request.get("method").string(), JSValue::function([](const JSValue& self, const Args&, const Args& c)->JSValue { event(c[0], {"super:get", self}); return JSValue::function([](const JSValue& self, const Args& args, const Args& c)->JSValue { event(c[0], {"super:call", self, array(args)}); return 7; }, c); }, {state})); auto mutableProto = proto; mutableProto.setPrototype(alternate);
            result.set("returned", normal(call(group, request.get("method").string().c_str()), state)); result.set("values", normal(group.get("_values"), state));
        }
    } catch (const JSTypeError&) { result.set("error", "TypeError"); } catch (const JSRangeError&) { result.set("error", "RangeError"); } catch (const std::exception&) { result.set("error", "Error"); }
    auto mutableProto = proto; mutableProto.setPrototype(oldParent); auto mutableCtor = ctor; mutableCtor.setPrototype(oldCtorParent); ThreeUtils::setConsoleFunction({}); result.set("events", state.get("events")); return result;
}
}
int main() { try { const std::string input(std::istreambuf_iterator<char>(std::cin), {}); const auto requests = Animation_detail::parseJSON(input); auto result = array(); for (unsigned i = 0; i < requests.length(); ++i) call(result, "push", {run(requests.get(std::to_string(i)))}); std::cout << result.toJSON()->dump(); return 0; } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; } }

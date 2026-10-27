#include "animation/AnimationUtils.h"
#include "core/JSWeakMap.h"
#include "nodes/core/NodeFrame.h"
#include "nodes/core/constants.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue call(const JSValue& value, const std::string& name, const Args& args = {}) {
    return value.get(name).call(value, args);
}
JSValue array(const Args& values) {
    auto out = JSValue::array();
    for (const auto& value : values) call(out, "push", {value});
    return out;
}
JSValue encode(const JSValue& value) {
    if (value.isUndefined()) return "#undefined";
    if (value.isSymbol()) return "#symbol";
    if (value.isCallable()) return "#function";
    if (value.isObject()) return "#object";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n)) return "#NaN";
        if (std::isinf(n)) return n > 0 ? "#Infinity" : "#-Infinity";
        if (n == 0 && std::signbit(n)) return "#-0";
    }
    return value;
}
void event(const JSValue& state, const Args& values) {
    call(state.get("events"), "push", {array(values)});
}
void data(JSValue object, const std::string& key, const JSValue& value, bool enumerable = true) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.configurable = true;
    descriptor.enumerable = enumerable;
    object.defineProperty(key, descriptor);
}
void accessor(JSValue object, const std::string& key, const JSValue& getter,
              const JSValue& setter = {}) {
    JSPropertyDescriptor descriptor;
    descriptor.getter = getter;
    descriptor.setter = setter;
    descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue decode(const JSValue& spec, const JSValue& state = {}, const std::string& label = "") {
    if (!spec.isObject()) return spec;
    const auto tag = spec.get("tag").toString();
    if (tag == "undefined") return {};
    if (tag == "nan") return std::numeric_limits<double>::quiet_NaN();
    if (tag == "infinity") return std::numeric_limits<double>::infinity();
    if (tag == "negative-zero") return -0.0;
    if (tag == "symbol") return JSValue::symbol("fixture");
    auto object = JSValue::object();
    if (tag == "coerce") {
        object.setPrimitiveConverter(JSValue::function(
            [](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                event(captures[0], {"coerce", captures[1], args.at(0)});
                if (captures[2].get("throw").toBoolean()) throw JSTypeError("coercion");
                return decode(captures[2].get("value"));
            }, {state, label, spec}));
    }
    return object;
}
std::string phase(const JSValue& request) { return request.get("phase").toString(); }
std::string method(const std::string& stage) {
    return stage == "before" ? "updateBeforeNode" : stage == "after" ? "updateAfterNode" : "updateNode";
}
std::string callback(const std::string& stage) {
    return stage == "before" ? "updateBefore" : stage == "after" ? "updateAfter" : "update";
}
std::string typeMethod(const std::string& stage) {
    return stage == "before" ? "getUpdateBeforeType" : stage == "after" ? "getUpdateAfterType" : "getUpdateType";
}
std::string mapName(const std::string& stage) {
    return stage == "before" ? "updateBeforeMap" : stage == "after" ? "updateAfterMap" : "updateMap";
}
JSValue newState(const JSValue& request, const JSValue& frame) {
    auto state = JSValue::object();
    state.set("request", request);
    state.set("frame", frame);
    state.set("events", JSValue::array());
    state.set("calls", 0);
    state.set("ref", JSValue::object());
    return state;
}
JSValue recordSnapshot(const JSValue& record) {
    if (!record.isObject()) return encode(record);
    return array({encode(record.get("frameId")), encode(record.get("renderId")),
                  array([&] { Args values; for (const auto& key : record.keys()) values.emplace_back(key); return values; }())});
}
JSValue peek(const JSValue& frame, const std::string& stage, const JSValue& ref) {
    return JSWeakMap(frame.get(mapName(stage))).get(ref);
}
JSValue snapshot(const JSValue& state, bool includeMap = true) {
    const auto frame = state.get("frame");
    auto out = JSValue::object();
    for (const auto* key : {"time", "deltaTime", "frameId", "renderId", "lastTime"})
        out.set(key, encode(frame.get(key)));
    out.set("hasLastTime", frame.hasOwn("lastTime"));
    out.set("calls", state.get("calls"));
    if (includeMap) out.set("mark", recordSnapshot(peek(frame, phase(state.get("request")), state.get("ref"))));
    return out;
}
JSValue invoke(const JSValue& frame, const std::string& name, const Args& args = {}) {
    try { return encode(call(frame, name, args)); }
    catch (const JSTypeError&) { return "#TypeError"; }
    catch (const JSRangeError&) { return "#RangeError"; }
}
JSValue makeNode(JSValue state, const std::string& label = "node") {
    auto node = JSValue::object();
    const auto stage = phase(state.get("request"));
    node.set(typeMethod(stage), JSValue::function(
        [](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            event(captures[0], {"type", captures[1], self.sameValue(captures[0].get(captures[1].string())),
                                args.at(0).sameValue(captures[0].get("frame"))});
            return captures[0].get("request").get("type");
        }, {state, label}));
    node.set("updateReference", JSValue::function(
        [](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            event(captures[0], {"reference", captures[1], self.sameValue(captures[0].get(captures[1].string())),
                                args.at(0).sameValue(captures[0].get("frame"))});
            return captures[0].get("ref");
        }, {state, label}));
    node.set(callback(stage), JSValue::function(
        [](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto state = captures[0];
            auto frame = state.get("frame");
            const auto request = state.get("request");
            const auto count = state.get("calls").toNumber() + 1;
            state.set("calls", count);
            event(state, {"callback", captures[1], count,
                          self.sameValue(state.get(captures[1].string())), args.at(0).sameValue(frame),
                          encode(frame.get("frameId")), encode(frame.get("renderId"))});
            const auto behavior = request.get("behavior").toString();
            if (behavior == "throw" && count == 1) throw JSTypeError("callback");
            if (behavior == "mutate") frame.set(request.get("axis").string(), decode(request.get("nextId")));
            if (behavior == "reenter" && count == 1) {
                event(state, {"nestedResult", invoke(frame, method(phase(request)), {self})});
            }
            const auto results = request.get("returns");
            // Match request.returns?.[index] in the fixture's JavaScript node.
            // An omitted optional list returns undefined without a property read.
            if (results.isUndefined() || results.isNull()) return {};
            return decode(results.get(std::to_string(static_cast<std::uint32_t>(count - 1))));
        }, {state, label}));
    state.set(label, node);
    return node;
}
JSValue runBasic(const JSValue& request) {
    NodeFrame native;
    auto frame = native.object();
    auto state = newState(request, frame);
    auto node = makeNode(state);
    if (request.hasOwn("frameId")) frame.set("frameId", decode(request.get("frameId")));
    if (request.hasOwn("renderId")) frame.set("renderId", decode(request.get("renderId")));
    auto out = JSValue::object();
    auto steps = JSValue::array();
    auto second = request.get("shared").toBoolean() ? makeNode(state, "other") : node;
    const auto operations = request.get("steps");
    for (std::uint32_t i = 0; i < operations.length(); ++i) {
        const auto operation = operations.get(std::to_string(i));
        for (const auto* key : {"frameId", "renderId"})
            if (operation.hasOwn(key)) frame.set(key, decode(operation.get(key)));
        const auto selected = operation.get("other").toBoolean() ? second : node;
        auto result = JSValue::object();
        result.set("result", invoke(frame, method(phase(request)), {selected}));
        result.set("state", snapshot(state));
        call(steps, "push", {result});
    }
    out.set("steps", steps);
    out.set("events", state.get("events"));
    return out;
}
JSValue runOrder(const JSValue& request) {
    NodeFrame native;
    auto frame = native.object();
    auto state = newState(request, frame);
    frame.set("frameId", 1);
    frame.set("renderId", 2);
    auto node = JSValue::object();
    state.set("node", node);
    auto maps = JSValue::object();
    maps.set("frameId", 0);
    maps.set("renderId", 0);
    state.set("record", maps);
    auto fakeMap = JSValue::object();
    state.set("fakeMap", fakeMap);
    for (const auto& name : {typeMethod(phase(request)), std::string("updateReference"), callback(phase(request)), std::string("_getMaps")}) {
        auto owner = name == "_getMaps" ? frame : node;
        accessor(owner, name, JSValue::function(
            [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
                auto state = captures[0];
                const auto name = captures[1].string();
                const auto owner = name == "_getMaps" ? state.get("frame") : state.get("node");
                event(state, {"get", name, receiver.sameValue(owner)});
                if (state.get("request").get("throwAt").strictEquals("get:" + name)) throw JSTypeError("getter");
                return JSValue::function(
                    [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                        auto state = captures[0];
                        const auto name = captures[1].string();
                        const auto request = state.get("request");
                        const auto stage = phase(request);
                        const auto owner = name == "_getMaps" ? state.get("frame") : state.get("node");
                        event(state, {"call", name, receiver.sameValue(owner),
                                      args.at(0).sameValue(name == "_getMaps" ? state.get("fakeMap") : state.get("frame"))});
                        if (request.get("throwAt").strictEquals("call:" + name)) throw JSTypeError("call");
                        if (name == typeMethod(stage)) return request.get("type");
                        if (name == "updateReference") return state.get("ref");
                        if (name == "_getMaps") {
                            event(state, {"referenceArgument", args.at(1).sameValue(state.get("ref"))});
                            return state.get("record");
                        }
                        state.set("calls", state.get("calls").toNumber() + 1);
                        if (request.get("changeId").toBoolean()) state.get("frame").set("frameId", 9);
                        return decode(request.get("result"));
                    }, captures, name);
            }, {state, name}));
    }
    accessor(frame, mapName(phase(request)), JSValue::function(
        [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
            event(captures[0], {"mapArgument", receiver.sameValue(captures[0].get("frame"))});
            if (captures[0].get("request").get("throwAt").strictEquals("map")) throw JSTypeError("map");
            return captures[0].get("fakeMap");
        }, {state}));
    for (const auto* name : {"frameId", "renderId"}) {
        state.set(std::string("stored:") + name, maps.get(name));
        accessor(maps, name, JSValue::function(
            [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
                event(captures[0], {"markGet", captures[1], receiver.sameValue(captures[0].get("record"))});
                if (captures[0].get("request").get("throwAt").strictEquals("markGet")) throw JSTypeError("mark");
                return captures[0].get("stored:" + captures[1].string());
            }, {state, name}), JSValue::function(
            [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                event(captures[0], {"markSet", captures[1], encode(args.at(0)), receiver.sameValue(captures[0].get("record"))});
                if (captures[0].get("request").get("throwAt").strictEquals("markSet")) throw JSTypeError("mark");
                auto state = captures[0];
                state.set("stored:" + captures[1].string(), args.at(0));
                return {};
            }, {state, name}));
    }
    auto out = JSValue::object();
    out.set("result", invoke(frame, method(phase(request)), {node}));
    out.set("frameId", encode(frame.get("frameId")));
    out.set("marks", array({encode(state.get("stored:frameId")), encode(state.get("stored:renderId"))}));
    out.set("calls", state.get("calls"));
    out.set("events", state.get("events"));
    return out;
}
JSValue runMaps(const JSValue& request) {
    NodeFrame native;
    auto frame = native.object();
    auto state = newState(request, frame);
    auto map = createNativeWeakMap();
    auto key = decode(request.get("key"));
    state.set("key", key);
    state.set("map", map);
    const auto originalGet = map.get("get");
    const auto originalSet = map.get("set");
    state.set("originalGet", originalGet);
    state.set("originalSet", originalSet);
    if (request.hasOwn("initial")) originalSet.call(map, {key, decode(request.get("initial"))});
    auto setFunction = JSValue::function(
        [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
            auto state = captures[0];
            event(state, {captures[1], receiver.sameValue(state.get("map")), args.at(0).sameValue(state.get("key")), recordSnapshot(args.at(1))});
            if (state.get("request").get("throwAt").strictEquals(captures[1])) throw JSTypeError("set");
            return state.get("originalSet").call(receiver, args);
        }, {state, "set"});
    state.set("setFunction", setFunction);
    accessor(map, "set", JSValue::function(
        [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
            event(captures[0], {"get:set", receiver.sameValue(captures[0].get("map"))});
            if (captures[0].get("request").get("throwAt").strictEquals("get:set")) throw JSTypeError("set getter");
            return captures[0].get("setFunction");
        }, {state}));
    accessor(map, "get", JSValue::function(
        [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
            event(captures[0], {"get:get", receiver.sameValue(captures[0].get("map"))});
            if (captures[0].get("request").get("throwAt").strictEquals("get:get")) throw JSTypeError("get getter");
            return JSValue::function(
                [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                    auto state = captures[0];
                    event(state, {"get", receiver.sameValue(state.get("map")), args.at(0).sameValue(state.get("key"))});
                    if (state.get("request").get("throwAt").strictEquals("get")) throw JSTypeError("get");
                    if (state.get("request").get("replaceSet").toBoolean()) {
                        state.set("setFunction", JSValue::function(
                            [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                                event(captures[0], {"replacementSet", receiver.sameValue(captures[0].get("map")),
                                                   args.at(0).sameValue(captures[0].get("key")), recordSnapshot(args.at(1))});
                                return captures[0].get("originalSet").call(receiver, args);
                            }, {state}));
                    }
                    return state.get("originalGet").call(receiver, args);
                }, captures);
        }, {state}));
    auto out = JSValue::object();
    auto results = JSValue::array();
    for (int i = 0; i < 2; ++i) {
        try { call(results, "push", {recordSnapshot(call(frame, "_getMaps", {map, key}))}); }
        catch (const JSTypeError&) { call(results, "push", {"#TypeError"}); }
    }
    out.set("results", results);
    out.set("events", state.get("events"));
    return out;
}
JSValue runMapPrototype(const JSValue& request) {
    // Construct the frame before touching Object.prototype. The production
    // operation under test is only _getMaps' object literal initialization.
    NodeFrame native;
    auto frame = native.object();
    auto state = newState(request, frame);
    auto map = createNativeWeakMap();
    auto reference = JSValue::object();
    auto original = JSValue::object();
    data(original, "renderId", 23);
    data(original, "frameId", 47);
    if (request.get("hit").toBoolean()) call(map, "set", {reference, original});
    auto prototype = JSValue::object().getPrototype();
    std::vector<std::pair<std::string, std::optional<JSPropertyDescriptor>>> saved;
    const auto targets = request.get("targets");
    for (std::uint32_t i = 0; i < targets.length(); ++i) {
        const auto name = targets.get(std::to_string(i)).string();
        saved.emplace_back(name, prototype.getOwnPropertyDescriptor(name));
        JSPropertyDescriptor descriptor;
        descriptor.configurable = true;
        descriptor.enumerable = false;
        const auto pollution = request.get("pollution").string();
        if (pollution == "readonly") {
            descriptor.value = 88;
            descriptor.writable = false;
        } else if (pollution == "setter") {
            descriptor.setter = JSValue::function(
                [](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                    event(captures[0], {"prototypeSet", captures[1], encode(args.at(0))});
                    return {};
                }, {state, name});
        } else {
            descriptor.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& captures) -> JSValue {
                    event(captures[0], {"prototypeGet", captures[1]});
                    return 99;
                }, {state, name});
        }
        prototype.defineProperty(name, descriptor);
    }
    const auto restore = [&] {
        for (const auto& [name, descriptor] : saved) {
            if (descriptor) prototype.defineProperty(name, *descriptor);
            else prototype.erase(name);
        }
    };
    auto out = JSValue::object();
    try {
        JSValue record;
        try {
            record = call(frame, "_getMaps", {map, reference});
            out.set("result", recordSnapshot(record));
            out.set("sameOnSecondCall", call(frame, "_getMaps", {map, reference}).sameValue(record));
            out.set("sameAsInitial", record.sameValue(original));
            auto descriptors = JSValue::array();
            for (const auto* name : {"renderId", "frameId"}) {
                const auto descriptor = record.getOwnPropertyDescriptor(name);
                call(descriptors, "push", {descriptor ? array({name, encode(descriptor->value.value_or(JSValue{})),
                    descriptor->writable.value_or(false), descriptor->enumerable.value_or(false),
                    descriptor->configurable.value_or(false)}) : JSValue{}});
            }
            out.set("descriptors", descriptors);
        } catch (const JSTypeError&) {
            out.set("result", "#TypeError");
        }
        out.set("events", state.get("events"));
    } catch (...) {
        restore();
        throw;
    }
    restore();
    return out;
}
JSValue runClock(const JSValue& request) {
    NodeFrame native;
    auto frame = native.object();
    auto state = newState(request, frame);
    state.set("clockIndex", 0);
    state.set("nowGets", 0);
    auto performance = JSValue::object();
    state.set("performance", performance);
    accessor(performance, "now", JSValue::function(
        [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
            auto state = captures[0];
            const auto count = state.get("nowGets").toNumber() + 1;
            state.set("nowGets", count);
            event(state, {"get:now", count, receiver.sameValue(state.get("performance"))});
            if (state.get("request").get("throwAt").strictEquals("get:now:" + std::to_string(static_cast<int>(count)))) throw JSTypeError("now getter");
            return JSValue::function(
                [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                    auto state = captures[0];
                    const auto index = state.get("clockIndex").toNumber();
                    state.set("clockIndex", index + 1);
                    event(state, {"now", index + 1, receiver.sameValue(state.get("performance")), args.size()});
                    if (state.get("request").get("throwAt").strictEquals("now:" + std::to_string(static_cast<int>(index + 1)))) throw JSTypeError("now");
                    const auto spec = state.get("request").get("ticks").get(std::to_string(static_cast<int>(index)));
                    return decode(spec, state, "tick:" + std::to_string(static_cast<int>(index + 1)));
                }, captures);
        }, {state}));
    NodeFrame::setClock(performance);
    for (const auto* name : {"time", "deltaTime", "frameId", "renderId", "lastTime"}) {
        if (request.hasOwn(name)) frame.set(name, decode(request.get(name), state, name));
        state.set(std::string("field:") + name, frame.get(name));
        state.set(std::string("has:") + name, frame.hasOwn(name));
        if (request.get("accessors").toBoolean()) accessor(frame, name, JSValue::function(
            [](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
                event(captures[0], {"fieldGet", captures[1], receiver.sameValue(captures[0].get("frame"))});
                if (captures[0].get("request").get("throwAt").strictEquals("get:" + captures[1].string())) throw JSTypeError("field getter");
                return captures[0].get("field:" + captures[1].string());
            }, {state, name}), JSValue::function(
            [](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
                auto state = captures[0];
                event(state, {"fieldSet", captures[1], encode(args.at(0)), receiver.sameValue(state.get("frame"))});
                if (state.get("request").get("throwAt").strictEquals("set:" + captures[1].string())) throw JSTypeError("field setter");
                state.set("field:" + captures[1].string(), args.at(0));
                state.set("has:" + captures[1].string(), true);
                return {};
            }, {state, name}));
    }
    auto results = JSValue::array();
    const auto count = request.get("count").toNumber();
    for (int i = 0; i < count; ++i) {
        auto row = JSValue::object();
        row.set("result", invoke(frame, "update"));
        for (const auto* name : {"time", "deltaTime", "frameId", "renderId", "lastTime"})
            row.set(name, encode(request.get("accessors").toBoolean() ? state.get(std::string("field:") + name) : frame.get(name)));
        row.set("hasLastTime", frame.hasOwn("lastTime"));
        call(results, "push", {row});
    }
    auto out = JSValue::object();
    out.set("results", results);
    out.set("events", state.get("events"));
    return out;
}
JSValue runMetadata() {
    NodeFrame native;
    auto frame = native.object();
    auto prototype = NodeFrame::prototype();
    auto out = JSValue::object();
    Args names;
    for (const auto& name : frame.ownPropertyNames()) names.emplace_back(name);
    out.set("own", array(names));
    names.clear();
    for (const auto& name : frame.keys()) names.emplace_back(name);
    out.set("keys", array(names));
    names.clear();
    for (const auto& name : prototype.ownPropertyNames()) names.emplace_back(name);
    out.set("prototypeOwn", array(names));
    out.set("prototypeMatches", frame.getPrototype().sameValue(prototype));
    out.set("constructorName", prototype.get("constructor").get("name"));
    out.set("constructorLength", prototype.get("constructor").get("length"));
    out.set("constructorPrototype", prototype.get("constructor").get("prototype").sameValue(prototype));
    out.set("callConstructor", invoke(prototype, "constructor"));
    auto fields = JSValue::array();
    for (const auto& name : frame.ownPropertyNames()) {
        const auto descriptor = frame.getOwnPropertyDescriptor(name).value();
        call(fields, "push", {array({name, encode(frame.get(name)), descriptor.writable.value_or(false),
                                    descriptor.enumerable.value_or(false), descriptor.configurable.value_or(false)})});
    }
    out.set("fields", fields);
    auto methods = JSValue::array();
    for (const auto& name : prototype.ownPropertyNames()) {
        const auto descriptor = prototype.getOwnPropertyDescriptor(name).value();
        const auto fn = prototype.get(name);
        call(methods, "push", {array({name, fn.get("name"), fn.get("length"),
                                     descriptor.writable.value_or(false), descriptor.enumerable.value_or(false), descriptor.configurable.value_or(false),
                                     fn.isConstructible(), fn.hasOwn("prototype")})});
    }
    out.set("methods", methods);
    out.set("mapsDistinct", !frame.get("updateMap").sameValue(frame.get("updateBeforeMap")) &&
                            !frame.get("updateMap").sameValue(frame.get("updateAfterMap")) &&
                            !frame.get("updateBeforeMap").sameValue(frame.get("updateAfterMap")));
    out.set("mapBrands", array({isNativeWeakMap(frame.get("updateMap")), isNativeWeakMap(frame.get("updateBeforeMap")), isNativeWeakMap(frame.get("updateAfterMap"))}));
    return out;
}
JSValue runGC() {
    auto out = JSValue::array();
    for (const auto* stage : {"before", "update", "after"}) {
        NodeFrame native;
        auto frame = native.object();
        frame.set("frameId", 1);
        JSWeakValue weakKey, weakRecord, weakPayload;
        JSValue liveKey;
        {
            auto key = JSValue::object();
            key.set("self", key);
            auto payload = JSValue::object();
            payload.set("self", payload);
            weakKey = JSWeakValue(key);
            weakPayload = JSWeakValue(payload);
            auto record = call(frame, "_getMaps", {frame.get(mapName(stage)), key});
            record.set("payload", payload);
            record.set("key", key);
            weakRecord = JSWeakValue(record);
            liveKey = key;
        }
        frame.collectGarbage();
        call(out, "push", {array({stage, !weakKey.expired(), !weakRecord.expired(), !weakPayload.expired()})});
        liveKey = {};
        frame.collectGarbage();
        call(out, "push", {array({stage, !weakKey.expired(), !weakRecord.expired(), !weakPayload.expired()})});
    }
    auto keeper = JSValue::object();
    JSWeakValue weakFrame, weakKey, weakRecord;
    JSValue liveKey;
    {
        NodeFrame native;
        auto frame = native.object();
        auto key = JSValue::object();
        liveKey = key;
        auto record = call(frame, "_getMaps", {frame.get("updateMap"), key});
        record.set("frame", frame);
        frame.set("self", frame);
        weakFrame = JSWeakValue(frame);
        weakKey = JSWeakValue(key);
        weakRecord = JSWeakValue(record);
    }
    keeper.collectGarbage();
    call(out, "push", {array({"frameDropped", !weakFrame.expired(), !weakKey.expired(), !weakRecord.expired()})});
    liveKey = {};
    keeper.collectGarbage();
    call(out, "push", {array({"keyDropped", !weakFrame.expired(), !weakKey.expired(), !weakRecord.expired()})});
    return out;
}
JSValue mutableValue(const JSValue& spec,const JSValue& state){
    return spec.isObject()&&spec.get("tag").strictEquals("match")?state.get("type"):decode(spec,state,"enum");
}
void mutablePoint(JSValue state,const std::string& point){
    event(state,{"point",point,state.get("depth")});const auto request=state.get("request");
    if(request.get("throwAt").strictEquals(point))throw JSTypeError(point);
    if(request.get("mutateAt").strictEquals(point)){
        const auto changes=request.get("changes");for(const auto& key:changes.keys())state.get("enumValues").set(key,mutableValue(changes.get(key),state));
        event(state,{"mutated",point});
    }
    if(request.get("gc").toBoolean()&&!state.get("gcScanned").toBoolean()){
        state.set("gcScanned",true);state.collectGarbage();
    }
    if(request.get("reenterAt").strictEquals(point)&&!state.get("reentered").toBoolean()){
        state.set("reentered",true);state.set("depth",1);
        const auto result=invoke(state.get("frame"),method(phase(request)),{state.get("node")});state.set("depth",0);event(state,{"nested",point,result});
    }
}
JSValue runMutable(const JSValue& request){
    NodeFrame native;auto frame=native.object(),state=newState(request,frame),node=JSValue::object();state.set("node",node);state.set("depth",0);
    state.set("type",decode(request.get("type"),state,"type"));state.set("enumValues",JSValue::object());state.set("enumReads",JSValue::object());
    state.set("field:frameId",request.hasOwn("frameId")?decode(request.get("frameId")):JSValue(1));state.set("field:renderId",request.hasOwn("renderId")?decode(request.get("renderId")):JSValue(2));
    const auto updateTypes=nativeNodeConstantsBinding("NodeUpdateType");const auto constants=nativeNodeConstants();const auto oldFacade=constants.getOwnPropertyDescriptor("NodeUpdateType");
    std::vector<JSPropertyDescriptor> saved;for(const auto* key:{"FRAME","RENDER","OBJECT"}){
        saved.push_back(updateTypes.getOwnPropertyDescriptor(key).value());state.get("enumReads").set(key,0);
        state.get("enumValues").set(key,request.get("enums").hasOwn(key)?mutableValue(request.get("enums").get(key),state):JSValue(key==std::string("FRAME")?"frame":key==std::string("RENDER")?"render":"object"));
        accessor(updateTypes,key,JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
            auto state=c[0],reads=state.get("enumReads");const auto key=c[1].string();const auto count=reads.get(key).toNumber()+1;reads.set(key,count);
            event(state,{"enumGet",key,count,state.get("depth"),self.sameValue(c[2])});mutablePoint(state,"enum:"+key);
            const auto sequences=state.get("request").get("sequences");const auto sequence=sequences.isObject()?sequences.get(key):JSValue{};
            if(sequence.type()==JSValue::Type::Array&&sequence.length()>0){const auto index=std::min(count-1,static_cast<double>(sequence.length()-1));return mutableValue(sequence.get(JSValue(index)),state);}
            return state.get("enumValues").get(key);
        },{state,key,updateTypes}));
    }
    if(request.get("replaceFacade").toBoolean()){auto facade=constants;facade.set("NodeUpdateType",JSValue::object());}
    for(const auto* key:{"frameId","renderId"})accessor(frame,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{mutablePoint(c[0],"frameGet:"+c[1].toString());return c[0].get("field:"+c[1].toString());},{state,key}),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{mutablePoint(c[0],"frameSet:"+c[1].toString());auto state=c[0];state.set("field:"+c[1].toString(),a.at(0));return {};},{state,key}));
    const auto originalMaps=frame.get("_getMaps");const auto stage=phase(request);state.set("originalMaps",originalMaps);
    const auto originalMap=frame.get(mapName(stage));state.set("originalMap",originalMap);
    accessor(frame,mapName(stage),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{mutablePoint(c[0],"mapArgument");return c[0].get("originalMap");},{state}));
    accessor(frame,"_getMaps",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        mutablePoint(c[0],"maps.get");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
            auto state=c[0];mutablePoint(state,"maps.call");event(state,{"mapsArguments",self.sameValue(state.get("frame")),a.at(0).sameValue(state.get("originalMap")),a.at(1).sameValue(state.get("ref"))});
            auto maps=state.get("originalMaps").call(self,a);
            if(!state.get("record").sameValue(maps)){
                state.set("record",maps);for(const auto* key:{"frameId","renderId"}){
                    state.set(std::string("stored:")+key,maps.get(key));accessor(maps,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{mutablePoint(c[0],"markGet:"+c[1].toString());return c[0].get("stored:"+c[1].toString());},{state,key}),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{mutablePoint(c[0],"markSet:"+c[1].toString());event(c[0],{"markValue",c[1],encode(a.at(0))});auto state=c[0];state.set("stored:"+c[1].toString(),a.at(0));return {};},{state,key}));
                }
            }return maps;
        },c);
    },{state}));
    for(const auto& name:{typeMethod(stage),std::string("updateReference"),callback(stage)}){
        const auto role=name==typeMethod(stage)?"type":name=="updateReference"?"reference":"callback";
        accessor(node,name,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            mutablePoint(c[0],c[1].toString()+".get");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                auto state=c[0];const auto role=c[1].string();mutablePoint(state,role+".call");event(state,{"receiver",role,self.sameValue(state.get("node")),a.at(0).sameValue(state.get("frame"))});
                if(role=="type")return state.get("type");if(role=="reference")return state.get("ref");
                const auto count=state.get("calls").toNumber()+1;state.set("calls",count);const auto returns=state.get("request").get("returns");return decode(returns.get(JSValue(count-1)),state);
            },c);
        },{state,role}));
    }
    auto restore=[&]{std::size_t i=0;for(const auto* key:{"FRAME","RENDER","OBJECT"}){auto target=updateTypes;target.defineProperty(key,saved[i++]);}if(oldFacade){auto facade=constants;facade.defineProperty("NodeUpdateType",*oldFacade);}};
    try{
        auto steps=JSValue::array();const auto operations=request.get("steps");for(unsigned i=0;i<operations.length();++i){const auto operation=operations.get(JSValue(i));
            for(const auto* key:{"frameId","renderId"})if(operation.hasOwn(key))frame.set(key,decode(operation.get(key)));
            if(operation.hasOwn("type"))state.set("type",decode(operation.get("type"),state,"stepType"));
            if(operation.hasOwn("enums"))for(const auto& key:operation.get("enums").keys())state.get("enumValues").set(key,mutableValue(operation.get("enums").get(key),state));
            auto row=JSValue::object();row.set("result",invoke(frame,method(stage),{node}));row.set("calls",state.get("calls"));row.set("ids",array({encode(state.get("field:frameId")),encode(state.get("field:renderId"))}));
            row.set("marks",state.get("record").isUndefined()?JSValue("#undefined"):array({encode(state.get("stored:frameId")),encode(state.get("stored:renderId"))}));call(steps,"push",{row});
        }
        auto out=JSValue::object();out.set("steps",steps);out.set("events",state.get("events"));out.set("reads",state.get("enumReads"));restore();return out;
    }catch(...){restore();throw;}
}
JSValue run(const JSValue& request) {
    const auto kind = request.get("kind").string();
    if (kind == "metadata") return runMetadata();
    if (kind == "basic") return runBasic(request);
    if (kind == "order") return runOrder(request);
    if (kind == "maps") return runMaps(request);
    if (kind == "mapPrototype") return runMapPrototype(request);
    if (kind == "clock") return runClock(request);
    if (kind == "mutable") return runMutable(request);
    throw JSTypeError("unknown probe kind");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "gc") {
            std::cout << runGC().toJSON()->dump();
            return 0;
        }
        const std::string input(std::istreambuf_iterator<char>(std::cin), {});
        const auto requests = Animation_detail::parseJSON(input);
        auto output = JSValue::array();
        for (std::uint32_t i = 0; i < requests.length(); ++i)
            call(output, "push", {run(requests.get(std::to_string(i)))});
        std::cout << output.toJSON()->dump();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

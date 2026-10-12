#include "animation/AnimationClip.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/ColorKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/StringKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "math/interpolants/LinearInterpolant.h"
#include <iostream>
#include <limits>

using A = AnimationArray;
namespace {
JSValue decode(JSValue value) {
    if (value.type() == JSValue::Type::Object && value.hasOwn("$undefined")) return JSValue{};
    if (value.type() == JSValue::Type::Object && value.hasOwn("$number")) {
        const auto name = value.get("$number").string();
        if (name == "NaN") return std::numeric_limits<double>::quiet_NaN();
        if (name == "Infinity") return std::numeric_limits<double>::infinity();
        if (name == "-Infinity") return -std::numeric_limits<double>::infinity();
        return -0.0;
    }
    if (value.type() == JSValue::Type::Array || value.type() == JSValue::Type::Object)
        for (const auto& key : value.keys()) value.set(key, decode(value.get(key)));
    return value;
}
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "$undefined";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n)) return "$NaN";
        if (std::isinf(n)) return n < 0 ? "$-Infinity" : "$Infinity";
        if (n == 0 && std::signbit(n)) return "$-0";
        return n;
    }
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array array;
        for (std::uint32_t i = 0; i < value.length(); ++i) array.push_back(encode(value.get(std::to_string(i))));
        return array;
    }
    if (value.type() == JSValue::Type::Object) {
        JsonValue::Object object;
        for (const auto& key : value.keys()) object.emplace(key, encode(value.get(key)));
        return object;
    }
    return value.toJSON().value_or(JsonValue("$undefined"));
}
const std::vector<std::string> types{"int8", "uint8", "int16", "uint16", "int32", "uint32", "float32", "float64", "clamped", "array"};
AnimationArrayType arrayType(const std::string& name) {
    for (std::size_t i = 0; i < types.size(); ++i) if (types[i] == name) return static_cast<AnimationArrayType>(i);
    throw std::runtime_error("Unknown fixture array type");
}
A array(JSValue value) {
    if (value.type() == JSValue::Type::Object && value.hasOwn("kind"))
        return convertArray(value.get("data"), arrayType(value.get("kind").string()));
    return value;
}
JsonValue dumpArray(const A& array) {
    if (const auto raw = std::get_if<JSValue>(&array.storage()); raw && raw->type() != JSValue::Type::Array) return encode(*raw);
    JsonValue::Array result;
    for (double i = 0, n = Animation_detail::length(array); i < n; ++i) result.push_back(encode(array.get(i)));
    return result;
}
std::shared_ptr<KeyframeTrack> makeTrack(const std::string& type, JSValue name, A times, A values, JSValue interpolation) {
    if (type == "base") return std::make_shared<KeyframeTrack>(name, times, values, interpolation);
    if (type == "number") return std::make_shared<NumberKeyframeTrack>(name, times, values, interpolation);
    if (type == "color") return std::make_shared<ColorKeyframeTrack>(name, times, values, interpolation);
    if (type == "vector") return std::make_shared<VectorKeyframeTrack>(name, times, values, interpolation);
    if (type == "quaternion") return std::make_shared<QuaternionKeyframeTrack>(name, times, values, interpolation);
    if (type == "bool") return std::make_shared<BooleanKeyframeTrack>(name, times, values, interpolation);
    return std::make_shared<StringKeyframeTrack>(name, times, values, interpolation);
}
JsonValue trackState(const KeyframeTrack& track) {
    JsonValue result = JsonValue::Object{{"name", encode(track.name)}, {"type", track.ValueTypeName},
        {"timeType", types[track.times.storage().index()]}, {"valueType", types[track.values.storage().index()]},
        {"times", dumpArray(track.times)}, {"values", dumpArray(track.values)},
        {"valueSize", encode(track.getValueSize())}, {"interpolation", encode(track.getInterpolation())},
        {"defaultInterpolation", encode(track.DefaultInterpolation)}, {"valid", track.validate()},
        {"json", encode(KeyframeTrack::toJSON(track))}};
    return result;
}
JsonValue samples(KeyframeTrack& track, JSValue times) {
    auto interpolant = track.createInterpolant();
    JsonValue::Array result;
    for (std::uint32_t i = 0; i < times.length(); ++i) result.push_back(dumpArray(interpolant->evaluate(times.get(std::to_string(i)).toNumber())));
    return result;
}
JsonValue trackCase(JSValue request) {
    auto times = array(request.get("times")), values = array(request.get("values"));
    auto track = makeTrack(request.get("type").string(), request.get("name"), times, values, request.get("interpolation"));
    if (request.has("settings")) {
        const auto settings = request.get("settings");
        if (settings.isNull()) track->settings = AnimationSettings(settings);
        else track->settings = AnimationSettings(array(settings.get("inTangents")), array(settings.get("outTangents")));
    }
    JsonValue output = JsonValue::Object{{"sameTimes", track->times.sameIdentity(times)}, {"sameValues", track->values.sameIdentity(values)}, {"initial", trackState(*track)}};
    JsonValue::Array states;
    const auto actions = request.get("actions");
    if (!actions.isUndefined()) for (std::uint32_t i = 0; i < actions.length(); ++i) {
        const auto action = actions.get(std::to_string(i)); const auto op = action.get("op").string();
        if (op == "shift") track->shift(action.get("value"));
        else if (op == "scale") track->scale(action.get("value"));
        else if (op == "trim") track->trim(action.get("start").toNumber(), action.get("end").toNumber());
        else if (op == "optimize") track->optimize();
        else if (op == "interpolation") track->setInterpolation(action.get("value"));
        else if (op == "clearFactory") track->createInterpolant = InterpolantFactory::FunctionPtr{};
        else if (op == "times") track->times = array(action.get("value"));
        else if (op == "sample") { states.push_back(samples(*track, action.get("times"))); continue; }
        else if (op == "clone") {
            const auto clone = track->clone();
            if (action.has("change")) clone->values.set(0, action.get("change"));
            states.emplace_back(JsonValue::Object{{"sameTimes", clone->times.sameIdentity(track->times)},
                {"sameValues", clone->values.sameIdentity(track->values)}, {"sameFactory", clone->createInterpolant.function() == track->createInterpolant.function()},
                {"state", trackState(*clone)}}); continue;
        }
        states.push_back(trackState(*track));
    }
    output["states"] = states; output["sourceTimes"] = dumpArray(times); output["sourceValues"] = dumpArray(values);
    return output;
}
JsonValue clipState(const AnimationClip& clip) {
    return JsonValue::Object{{"json", encode(clip.toJSON())}, {"valid", clip.validate()}};
}
void markGLTF(AnimationClip& clip) {
    for (const auto& track : *clip.tracks) track->createInterpolant = InterpolantFactory::custom([](KeyframeTrack& self, A result) {
        return std::make_shared<LinearInterpolant>(self.times, self.values, self.getValueSize() / 3, result);
    }, true);
}
JsonValue clipCase(JSValue request) {
    auto input = request.get("json");
    auto clip = AnimationClip::parse(input);
    if (request.get("gltf").toBoolean()) markGLTF(*clip);
    JsonValue output = JsonValue::Object{{"initial", clipState(*clip)}, {"input", encode(input)}};
    JsonValue::Array states;
    const auto actions = request.get("actions");
    if (!actions.isUndefined()) for (std::uint32_t i = 0; i < actions.length(); ++i) {
        const auto action = actions.get(std::to_string(i)); const auto op = action.get("op").string();
        if (op == "trim") clip->trim();
        else if (op == "optimize") clip->optimize();
        else if (op == "reset") clip->resetDuration();
        else if (op == "clone") {
            auto clone = clip->clone(); clone->uuid = "derived";
            states.push_back(clipState(*clone)); continue;
        } else if (op == "subclip") {
            const auto fps = action.has("fps") ? action.get("fps").toNumber() : 30;
            auto part = subclip(*clip, "part", action.get("start").toNumber(), action.get("end").toNumber(), fps);
            part->uuid = "derived"; states.push_back(clipState(*part)); continue;
        } else if (op == "additive") {
            const auto frame = action.has("frame") ? action.get("frame").toNumber() : 0;
            const auto fps = action.has("fps") ? action.get("fps").toNumber() : 30;
            std::shared_ptr<AnimationClip> reference;
            if (action.has("reference")) reference = AnimationClip::parse(action.get("reference"));
            if (reference && request.get("gltf").toBoolean()) markGLTF(*reference);
            makeClipAdditive(*clip, frame, reference.get(), fps);
        }
        states.push_back(clipState(*clip));
    }
    output["states"] = states;
    return output;
}
KeyframeTrack* callbackTrack = nullptr;
AnimationClip* callbackClip = nullptr;
void append(JSValue array, JSValue value) { array.set(std::to_string(array.length()), value); }
void getter(JSValue object, const std::string& name, JSValue function) {
    JSPropertyDescriptor descriptor;
    descriptor.getter = function; descriptor.enumerable = true; descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JsonValue callbackCase(const std::string& kind) {
    auto trace = JSValue::array();
    NumberKeyframeTrack track(".x", A::array({0, 1}), A::array({2, 6}));
    callbackTrack = &track;
    struct Clear { ~Clear() { callbackTrack = nullptr; callbackClip = nullptr; } } clear;
    if (kind == "settings") {
        auto replacement = JSValue::fromJSON(JsonValue::Object{{"inTangents", JsonValue::Array{7, 8}}, {"outTangents", JsonValue::Array{9, 10}}});
        auto settings = JSValue::object();
        getter(settings, "inTangents", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "in"); callbackTrack->settings = c[1]; c[0].collectGarbage(); return c[2];
        }, {trace, replacement, JSValue::array()}));
        getter(settings, "outTangents", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "out"); return c[1];
        }, {trace, JSValue::array()}));
        track.settings = settings; settings = JSValue{};
        track.scale(2);
        return JsonValue::Object{{"trace", encode(trace)}, {"times", dumpArray(track.times)}, {"settings", encode(replacement)}};
    }
    if (kind == "slice") {
        StringKeyframeTrack text(".s", A::array({0, 1}), A::array({"a", "b"})); callbackTrack = &text;
        auto original = std::get<JSValue>(text.values.storage());
        getter(original, "0", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "first"); callbackTrack->values = c[1]; c[0].collectGarbage(); return "a";
        }, {trace, JSValue::fromJSON(JsonValue::Array{"newA", "newB"})}));
        original = JSValue{};
        const auto clone = text.clone();
        return JsonValue::Object{{"trace", encode(trace)}, {"clone", dumpArray(clone->values)}, {"source", dumpArray(text.values)}};
    }
    if (kind == "size") {
        auto count = JSValue::object();
        count.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "valueOf"); callbackTrack->times = A::array({0, 1, 2, 3}); return 6;
        }, {trace}));
        auto values = JSValue::object(), times = JSValue::object();
        getter(values, "length", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "valueLength"); return c[1];
        }, {trace, count}));
        getter(times, "length", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "timeLength"); return 2;
        }, {trace}));
        track.values = values; track.times = times;
        const auto size = track.getValueSize();
        return JsonValue::Object{{"trace", encode(trace)}, {"size", size}, {"times", dumpArray(track.times)}};
    }
    if (kind == "duration") {
        auto times = JSValue::object();
        getter(times, "length", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "length"); callbackTrack->times = A::array({0, 100}); c[0].collectGarbage(); return 2;
        }, {trace}));
        getter(times, "1", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "last"); return 7;
        }, {trace}));
        auto sharedTrack = std::make_shared<NumberKeyframeTrack>(".x", A::array({0, 1}), A::array({2, 6}));
        callbackTrack = sharedTrack.get(); sharedTrack->times = times; times = JSValue{};
        AnimationClip clip("clip", 0, std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{sharedTrack}));
        clip.resetDuration();
        return JsonValue::Object{{"trace", encode(trace)}, {"duration", clip.duration}, {"times", dumpArray(sharedTrack->times)}};
    }
    if (kind == "durationTracks") {
        auto first = std::make_shared<NumberKeyframeTrack>(".x", A::array({0, 1}), A::array({2, 6}));
        auto second = std::make_shared<NumberKeyframeTrack>(".y", A::array({0, 100}), A::array({2, 6}));
        auto times = JSValue::object(); times.set("1", 7);
        getter(times, "length", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "replace");
            const auto first = (*callbackClip->tracks)[0];
            auto next = std::make_shared<NumberKeyframeTrack>(".z", A::array({0, 20}), A::array({2, 6}));
            callbackClip->tracks = std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{first, next});
            return 2;
        }, {trace}));
        first->times = times;
        AnimationClip clip("clip", 0, std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{first, second}));
        callbackClip = &clip; clip.resetDuration();
        return JsonValue::Object{{"trace", encode(trace)}, {"duration", clip.duration}, {"second", encode((*clip.tracks)[1]->name)}};
    }
    if (kind == "iterator" || kind == "arrayLike") {
        const bool iterator = kind == "iterator";
        auto source = iterator ? JSValue::array(2) : JSValue::object();
        if (!iterator) source.set("length", 2);
        source.set("last", 5);
        auto element = JSValue::object();
        element.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "valueOf"); auto source = c[1]; source.set("last", 90); source.set("2", 90); return 3;
        }, {trace, source}));
        getter(source, "0", JSValue::function([](const JSValue& self, const auto&, const auto& c) -> JSValue {
            append(c[0], "get0"); if (c[2].toBoolean()) { auto copy = self; copy.set("2", 7); } return c[1];
        }, {trace, element, iterator}));
        getter(source, "1", JSValue::function([](const JSValue& self, const auto&, const auto& c) -> JSValue {
            append(c[0], "get1"); return self.get("last");
        }, {trace}));
        const auto converted = convertArray(source, AnimationArrayType::Float32);
        return JsonValue::Object{{"trace", encode(trace)}, {"values", dumpArray(converted)}, {"last", encode(source.get("last"))}};
    }
    if (kind == "spread") {
        auto times = JSValue::array(), values = JSValue::fromJSON(JsonValue::Array{42}), source = JSValue::array(2);
        getter(source, "0", JSValue::function([](const JSValue& self, const auto&, const auto& c) -> JSValue {
            append(c[0], c[1].length()); auto copy = self; copy.set("2", 7); return 3;
        }, {trace, values}));
        getter(source, "1", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], c[1].length()); return 5;
        }, {trace, values}));
        auto key = JSValue::object(), keys = JSValue::array(); key.set("time", 4); key.set("value", source); keys.set("0", key);
        flattenJSON(keys, times, values, "value");
        return JsonValue::Object{{"trace", encode(trace)}, {"times", encode(times)}, {"values", encode(values)}};
    }
    if (kind == "toArray") {
        auto times = JSValue::array(), values = JSValue::fromJSON(JsonValue::Array{42}), source = JSValue::object();
        source.set("x", 7);
        source.set("toArray", JSValue::function([](const JSValue& self, const auto& args, const auto& c) -> JSValue {
            append(c[0], self.get("x")); append(c[0], args[1]); auto destination = args[0]; destination.set(args[1].toString(), self.get("x")); return destination;
        }, {trace}));
        auto key = JSValue::object(), keys = JSValue::array(); key.set("time", 4); key.set("value", source); keys.set("0", key);
        flattenJSON(keys, times, values, "value");
        return JsonValue::Object{{"trace", encode(trace)}, {"times", encode(times)}, {"values", encode(values)}};
    }
    throw std::runtime_error("Unknown callback fixture");
}
JsonValue sortTraceCase(JSValue request) {
    auto backing = request.get("times"), times = JSValue::array(backing.length()), trace = JSValue::array();
    auto state = JSValue::object(); state.set("changed", false);
    for (std::uint32_t i = 0; i < backing.length(); ++i) {
        getter(times, std::to_string(i), JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            append(c[0], "get:" + c[2].toString());
            auto value = JSValue::object();
            value.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& values) -> JSValue {
                append(values[0], "number:" + values[2].toString());
                auto state = values[3];
                if (values[4].toBoolean() && !state.get("changed").toBoolean()) {
                    state.set("changed", true); auto backing = values[1]; backing.set("0", -100);
                }
                if (!state.get("collected").toBoolean()) {
                    state.set("collected", true); values[0].collectGarbage();
                }
                return values[5];
            }, {c[0], c[1], c[2], c[3], c[4], c[1].get(c[2].toString())}));
            return value;
        }, {trace, backing, double(i), state, request.get("mutate")}));
    }
    const auto order = getKeyframeOrder(times);
    return JsonValue::Object{{"order", dumpArray(order)}, {"trace", encode(trace)}, {"backing", encode(backing)}};
}
JsonValue run(JSValue request) {
    const auto mode = request.get("mode").string();
    if (mode == "track") return trackCase(request);
    if (mode == "clip") return clipCase(request);
    if (mode == "callback") return callbackCase(request.get("kind").string());
    if (mode == "sortTrace") return sortTraceCase(request);
    if (mode == "convert") {
        const auto source = array(request.get("array"));
        const auto converted = convertArray(source, arrayType(request.get("type").string()));
        return JsonValue::Object{{"same", source.sameIdentity(converted)}, {"type", types[converted.storage().index()]}, {"value", dumpArray(converted)}};
    }
    if (mode == "sort") {
        const auto times = array(request.get("times")), values = array(request.get("values"));
        const auto order = getKeyframeOrder(times);
        return JsonValue::Object{{"order", dumpArray(order)}, {"times", dumpArray(sortedArray(times, 1, order))},
                                {"values", dumpArray(sortedArray(values, request.get("stride").toNumber(), order))}};
    }
    if (mode == "flatten") {
        auto times = JSValue::array(), values = JSValue::array();
        flattenJSON(request.get("keys"), times, values, request.get("property").string());
        return JsonValue::Object{{"times", encode(times)}, {"values", encode(values)}};
    }
    if (mode == "morph") {
        const auto fps = request.get("fps").toNumber(); const auto loop = request.get("noLoop").toBoolean();
        if (!request.get("groups").toBoolean()) {
            auto clip = AnimationClip::CreateFromMorphTargetSequence("morph", request.get("targets"), fps, loop);
            clip->uuid = "morph"; return clipState(*clip);
        }
        const auto clips = AnimationClip::CreateClipsFromMorphTargetSequences(request.get("targets"), fps, loop);
        JsonValue::Array result;
        for (const auto& clip : clips) { clip->uuid = "morph"; result.push_back(clipState(*clip)); }
        return result;
    }
    if (mode == "json") {
        const auto value = Animation_detail::parseJSON(request.get("text").string());
        return JsonValue::Object{{"value", encode(value)}, {"text", *Animation_detail::stringifyJSON(value)}};
    }
    throw std::runtime_error("Unknown fixture mode");
}
}
int main() {
    const std::string input(std::istreambuf_iterator<char>(std::cin), {});
    const auto requests = decode(Animation_detail::parseJSON(input));
    JsonValue::Array output;
    for (std::uint32_t i = 0; i < requests.length(); ++i) {
        try { output.push_back(run(requests.get(std::to_string(i)))); }
        catch (const JSTypeError&) { output.emplace_back(JsonValue::Object{{"error", "TypeError"}}); }
        catch (const JSRangeError&) { output.emplace_back(JsonValue::Object{{"error", "RangeError"}}); }
        catch (const Animation_detail::JSONSyntaxError&) { output.emplace_back(JsonValue::Object{{"error", "SyntaxError"}}); }
        catch (const std::exception&) { output.emplace_back(JsonValue::Object{{"error", "Error"}}); }
    }
    std::cout << JsonValue(output).dump() << '\n';
}

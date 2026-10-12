#include "catch.hpp"
#include "animation/AnimationClip.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/StringKeyframeTrack.h"
#include "math/interpolants/LinearInterpolant.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include <limits>

using A = AnimationArray;
TEST_CASE("Animation tracks preserve typed and ordinary array identity", "[animation]") {
    Float32Array times({0, 1}), values({2, 6});
    NumberKeyframeTrack number(".value", times, values);
    REQUIRE(number.times.sameIdentity(times));
    REQUIRE(number.values.sameIdentity(values));
    const auto interpolant = number.createInterpolant();
    values[1] = 10;
    REQUIRE(interpolant->evaluate(.5).number(0) == 6);
    auto strings = JSValue::fromJSON(JsonValue::Array{"a", "b"});
    StringKeyframeTrack text(".text", times, strings, InterpolateLinear);
    REQUIRE(text.values.sameIdentity(strings));
    REQUIRE(text.getInterpolation().strictEquals(InterpolateDiscrete));
    REQUIRE(text.createInterpolant()->evaluate(.9).get(0).string() == "a");
    const auto json = KeyframeTrack::toJSON(text);
    REQUIRE(json.get("values").sameValue(strings));
    REQUIRE_FALSE(json.get("times").sameValue(std::get<JSValue>(convertArray(times, AnimationArrayType::Array).storage())));
}
TEST_CASE("Animation optimization copies shared arrays and preserves upstream duplicate handling", "[animation]") {
    Float32Array times({0, 0, 1, 2, 3, 3}), values({0, 9, 1, 1, 1, 5});
    NumberKeyframeTrack track(".x", times, values);
    REQUIRE(&track.optimize() == &track);
    REQUIRE_FALSE(track.times.sameIdentity(times));
    REQUIRE(times.size() == 6);
    REQUIRE(values.get(1).number() == 9);
    REQUIRE(track.times.size() == 3);
    REQUIRE(track.times.number(0) == 0);
    REQUIRE(track.times.number(1) == 1);
    REQUIRE(track.times.number(2) == 3);
    track.trim(100, 101);
    REQUIRE(track.times.size() == 1);
    REQUIRE(track.times.number(0) == 3);
    REQUIRE(track.values.number(0) == 5);
}
TEST_CASE("Bezier track transformations retain original tangent behavior", "[animation]") {
    NumberKeyframeTrack track(".x", A::array({0, 1, 2}), A::array({0, 2, 4}), InterpolateBezier);
    Float32Array in({0, 0, .8f, 1, 1.8f, 3}), out({.2f, 1, 1.2f, 3, 2, 4});
    track.settings = AnimationSettings(in, out);
    auto interpolant = track.createInterpolant();
    track.shift(2).scale(2);
    REQUIRE(track.times.number(0) == 4);
    REQUIRE(in.get(2).number() == double(float(.8f * 2)));
    REQUIRE(in.get(3).number() == 1);
    const auto clone = track.clone();
    REQUIRE_FALSE(clone->settings.sameIdentity(track.settings));
    REQUIRE_FALSE(clone->settings.get("inTangents").sameIdentity(in));
    track.trim(5, 6);
    REQUIRE(track.settings.get("inTangents").sameIdentity(in));
    REQUIRE(in.size() == 6);
}
TEST_CASE("Cloned interpolant factories retain identity and use the clone receiver", "[animation]") {
    NumberKeyframeTrack original(".x", A::array({0, 1}), A::array({1, 3}));
    original.createInterpolant = InterpolantFactory::custom([](KeyframeTrack& receiver, A result) {
        return std::make_shared<LinearInterpolant>(receiver.times, receiver.values, receiver.getValueSize(), result);
    }, true);
    auto clone = original.clone();
    REQUIRE(clone->createInterpolant.function() == original.createInterpolant.function());
    REQUIRE(clone->createInterpolant.function()->isInterpolantFactoryMethodGLTFCubicSpline);
    REQUIRE(clone->getInterpolation().isUndefined());
    clone->values.set(1, 11);
    REQUIRE(clone->createInterpolant()->evaluate(.5).number(0) == 6);
    REQUIRE(original.createInterpolant()->evaluate(.5).number(0) == 2);
    QuaternionKeyframeTrack quaternion(".q", A::array({0, 1}), A::array({0, 0, 0, 1, 0, 1, 0, 0}));
    NumberKeyframeTrack numeric(".q", quaternion.times, quaternion.values);
    numeric.createInterpolant = quaternion.createInterpolant;
    REQUIRE(numeric.getInterpolation().isUndefined());
    REQUIRE(numeric.createInterpolant()->evaluate(.5).number(1) == Approx(std::sqrt(.5)));
}
TEST_CASE("AnimationClip parses frames and preserves JSON user data semantics", "[animation]") {
    const auto json = Animation_detail::parseJSON(R"({"name":"walk","uuid":"clip","fps":2,"tracks":[{"name":".x","type":"number","keys":[{"time":0,"value":2},{"time":4,"value":6}]}],"userData":"{\"ratio\":0.1,\"emoji\":\"\\ud83d\\udca9\"}"})");
    const auto clip = AnimationClip::parse(json);
    REQUIRE(clip->duration == 2);
    REQUIRE((*clip->tracks)[0]->times.number(1) == 2);
    REQUIRE(json.get("tracks").get("0").has("times"));
    REQUIRE(clip->toJSON().get("userData").string() == "{\"ratio\":0.1,\"emoji\":\"💩\"}");
    const auto clone = clip->clone();
    REQUIRE_FALSE(clone->tracks == clip->tracks);
    REQUIRE((*clone->tracks)[0] != (*clip->tracks)[0]);
    REQUIRE_FALSE(clone->userData.sameValue(clip->userData));
    REQUIRE_FALSE(clone->uuid.strictEquals(clip->uuid));
    REQUIRE_THROWS_AS(Animation_detail::parseJSON("[1,]"), Animation_detail::JSONSyntaxError);
    REQUIRE_THROWS_AS(Animation_detail::parseJSON("01"), Animation_detail::JSONSyntaxError);
}
TEST_CASE("JSON arrays create own data properties despite inherited index descriptors", "[animation][json]") {
    auto prototype = JSValue::array().getPrototype();
    struct RestorePrototype {
        JSValue prototype;
        std::optional<JSPropertyDescriptor> index;
        JSPropertyDescriptor length;
        ~RestorePrototype() {
            if (index) prototype.defineProperty("0", *index);
            else prototype.erase("0");
            prototype.defineProperty("length", length);
        }
    } restore{prototype, prototype.getOwnPropertyDescriptor("0"), *prototype.getOwnPropertyDescriptor("length")};
    auto calls = JSValue::object(nullptr); calls.set("count", 0);
    JSPropertyDescriptor inherited; inherited.configurable = true;
    SECTION("inherited setters are not invoked") {
        inherited.setter = JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                               const std::vector<JSValue>& captures) -> JSValue {
            auto calls = captures[0]; calls.set("count", calls.get("count").number() + 1); return {};
        }, {calls});
    }
    SECTION("inherited read-only values do not prevent parsing") {
        inherited.value = 99; inherited.writable = false;
    }
    prototype.defineProperty("0", inherited);
    const auto parsed = Animation_detail::parseJSON(R"([1,[2],{"nested":[3]},null])");
    REQUIRE(parsed.length() == 4);
    REQUIRE(parsed.get("0").strictEquals(1));
    REQUIRE(parsed.get("1").get("0").strictEquals(2));
    REQUIRE(parsed.get("2").get("nested").get("0").strictEquals(3));
    REQUIRE(parsed.get("3").isNull());
    REQUIRE(calls.get("count").strictEquals(0));
    const auto own = parsed.getOwnPropertyDescriptor("0");
    REQUIRE(own.has_value());
    REQUIRE(own->value->strictEquals(1));
    REQUIRE(own->writable.value_or(false));
    REQUIRE(own->enumerable.value_or(false));
    REQUIRE(own->configurable.value_or(false));
    REQUIRE_FALSE(own->setter.has_value());
}
TEST_CASE("Animation subclips and additive tracks follow original frame operations", "[animation]") {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    tracks->push_back(std::make_shared<NumberKeyframeTrack>(".x", A::array({0, 1, 2, 3}), A::array({2, 4, 6, 8})));
    tracks->push_back(std::make_shared<BooleanKeyframeTrack>(".visible", A::array({0, 3}), A::array({false, true})));
    AnimationClip original("clip", -1, tracks);
    REQUIRE(original.tracks == tracks);
    const auto part = subclip(original, "part", 30, 90);
    REQUIRE(part->tracks->size() == 1);
    REQUIRE(part->duration == 1);
    REQUIRE((*part->tracks)[0]->times.number(0) == 0);
    REQUIRE((*part->tracks)[0]->values.number(0) == 4);
    makeClipAdditive(original, 15);
    REQUIRE(original.blendMode.strictEquals(AdditiveAnimationBlendMode));
    REQUIRE((*original.tracks)[0]->values.number(0) == -1);
    REQUIRE((*original.tracks)[0]->values.number(3) == 5);
    REQUIRE((*original.tracks)[1]->values.get(0).strictEquals(false));
}
namespace {
class SerializedClip : public AnimationClip {
  public:
    using AnimationClip::AnimationClip;
    mutable int calls = 0;
    JSValue toJSON() const override {
        ++calls;
        auto result = AnimationClip::toJSON(*this);
        result.set("custom", true);
        return result;
    }
};
}
TEST_CASE("Scene caches a shared animation once across parent and children", "[animation][animation-scene]") {
    Scene scene;
    auto first = std::make_shared<Object3D>(), second = std::make_shared<Object3D>();
    auto clip = std::make_shared<SerializedClip>("shared");
    clip->uuid = "shared-clip";
    scene.animations.push_back(clip); first->animations.push_back(clip); second->animations.push_back(clip);
    scene.add(first); scene.add(second);
    const auto json = scene.toJSON();
    REQUIRE(clip->calls == 1);
    REQUIRE(json.at("animations").size() == 1);
    REQUIRE(json.at("animations")[0].at("custom").get<bool>());
    REQUIRE(json.at("object").at("animations")[0].get<std::string>() == "shared-clip");
    REQUIRE(json.at("object").at("children")[0].at("animations")[0].get<std::string>() == "shared-clip");
    REQUIRE(json.at("object").at("children")[1].at("animations")[0].get<std::string>() == "shared-clip");
}
TEST_CASE("Object3D reuses external animation caches and calls clip overrides", "[animation][animation-scene]") {
    Object3D first, second;
    auto clip = std::make_shared<SerializedClip>("shared"); clip->uuid = "clip-uuid";
    first.animations.push_back(clip); second.animations.push_back(clip);
    Object3DJSONContext context;
    const auto a = first.toJSON(&context), b = second.toJSON(&context);
    REQUIRE(clip->calls == 1);
    REQUIRE(context.animations.size() == 1);
    REQUIRE(context.animations.at("clip-uuid").at("custom").get<bool>());
    REQUIRE(a.at("object").at("animations") == b.at("object").at("animations"));
    REQUIRE_FALSE(a.contains("animations"));
    REQUIRE_FALSE(b.contains("animations"));
}
namespace { KeyframeTrack* reentrantTrack = nullptr; }
TEST_CASE("Animation keyframe order retains pinned V8 NaN and conversion order", "[animation]") {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto order = getKeyframeOrder(A::array({2, 1, nan, 0}));
    REQUIRE(order.number(0) == 3);
    REQUIRE(order.number(1) == 1);
    REQUIRE(order.number(2) == 0);
    REQUIRE(order.number(3) == 2);
    auto times = JSValue::array(), trace = JSValue::array();
    const double source[]{4, 1, 3, 2};
    for (std::size_t i = 0; i < 4; ++i) {
        auto value = JSValue::object();
        value.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            auto trace = c[0]; trace.set(std::to_string(trace.length()), c[1]); return c[2];
        }, {trace, double(i), source[i]}));
        times.set(std::to_string(i), value);
    }
    const auto observed = getKeyframeOrder(times);
    REQUIRE(trace.toJSON()->dump() == "[1,0,2,0,2,1,3,2,3,1]");
    REQUIRE(observed.number(0) == 1);
    REQUIRE(observed.number(1) == 3);
    REQUIRE(observed.number(2) == 2);
    REQUIRE(observed.number(3) == 0);
}
TEST_CASE("Animation tangent detection retains its original settings receiver during reentry", "[animation]") {
    NumberKeyframeTrack track(".x", A::array({0, 1}), A::array({2, 6}));
    reentrantTrack = &track;
    auto replacement = JSValue::fromJSON(JsonValue::Object{{"inTangents", JsonValue::Array{7, 8}}, {"outTangents", JsonValue::Array{9, 10}}});
    auto trace = JSValue::array(), original = JSValue::object();
    JSPropertyDescriptor in, out;
    in.getter = JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
        auto log = c[0]; log.set(std::to_string(log.length()), "in"); reentrantTrack->settings = c[1]; log.collectGarbage(); return JSValue::array();
    }, {trace, replacement});
    out.getter = JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
        auto log = c[0]; log.set(std::to_string(log.length()), "out"); return JSValue::array();
    }, {trace});
    original.defineProperty("inTangents", in); original.defineProperty("outTangents", out);
    track.settings = original; original = JSValue{};
    track.scale(2); reentrantTrack = nullptr;
    REQUIRE(trace.toJSON()->dump() == "[\"in\",\"out\"]");
    REQUIRE(track.settings.sameIdentity(AnimationSettings(replacement)));
    REQUIRE(track.settings.get("inTangents").number(0) == 14);
    REQUIRE(track.settings.get("outTangents").number(0) == 18);
    REQUIRE(track.times.number(1) == 2);
}
TEST_CASE("Animation typed conversion collects iterable values before coercion", "[animation]") {
    auto source = JSValue::array(2), element = JSValue::object();
    source.set("1", 5);
    element.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
        auto source = c[0]; source.set("1", 90); return 3;
    }, {source}));
    JSPropertyDescriptor first;
    first.getter = JSValue::function([](const JSValue& self, const auto&, const auto& c) -> JSValue {
        auto source = self; source.set("2", 7); return c[0];
    }, {element});
    source.defineProperty("0", first);
    const auto result = convertArray(source, AnimationArrayType::Float32);
    REQUIRE(result.size() == 3);
    REQUIRE(result.number(0) == 3);
    REQUIRE(result.number(1) == 5);
    REQUIRE(result.number(2) == 7);
    REQUIRE(source.get("1").number() == 90);
}
TEST_CASE("Animation lookup prioritizes legacy geometry clips including an empty list", "[animation][animation-scene]") {
    Mesh mesh;
    auto local = std::make_shared<AnimationClip>("walk"), geometry = std::make_shared<AnimationClip>("walk");
    mesh.animations.push_back(local);
    REQUIRE(AnimationClip::findByName(mesh, "walk") == local);
    mesh.geometry->animations = std::make_shared<AnimationClip::Clips>(AnimationClip::Clips{geometry});
    REQUIRE(AnimationClip::findByName(mesh, "walk") == geometry);
    mesh.geometry->animations->clear();
    REQUIRE(AnimationClip::findByName(mesh, "walk") == nullptr);
    REQUIRE(AnimationClip::findByName(*mesh.geometry->animations, "missing") == nullptr);
}

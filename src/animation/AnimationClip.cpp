#include "animation/AnimationClip.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/ColorKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/StringKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "math/MathUtils.h"
#include "core/Object3D.h"
#include "objects/Mesh.h"
#include <regex>

namespace {
std::shared_ptr<KeyframeTrack> trackAt(const AnimationClip::TracksPtr& tracks, std::size_t i) {
    if (!tracks || i >= tracks->size() || !(*tracks)[i]) throw JSTypeError("Cannot read properties of undefined track");
    return (*tracks)[i];
}
std::shared_ptr<KeyframeTrack> parseTrack(JSValue json) {
    if (json.get("type").isUndefined()) throw std::runtime_error("THREE.KeyframeTrack: track type undefined, can not parse");
    const auto originalType = json.get("type");
    if (originalType.type() != JSValue::Type::String) throw JSTypeError("typeName.toLowerCase is not a function");
    auto type = originalType.string();
    for (auto& c : type) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    enum Kind { Number, Vector, Color, Quaternion, Boolean, String } kind;
    if (type == "scalar" || type == "double" || type == "float" || type == "number" || type == "integer") kind = Number;
    else if (type == "vector" || type == "vector2" || type == "vector3" || type == "vector4") kind = Vector;
    else if (type == "color") kind = Color;
    else if (type == "quaternion") kind = Quaternion;
    else if (type == "bool" || type == "boolean") kind = Boolean;
    else if (type == "string") kind = String;
    else throw std::runtime_error("THREE.KeyframeTrack: Unsupported typeName: " + originalType.string());
    if (json.get("times").isUndefined()) {
        auto times = JSValue::array(), values = JSValue::array();
        flattenJSON(json.get("keys"), times, values, "value");
        json.set("times", times); json.set("values", values);
    }
    const auto name = json.get("name"), times = json.get("times"), values = json.get("values"), interpolation = json.get("interpolation");
    std::shared_ptr<KeyframeTrack> track;
    switch (kind) {
    case Number: track = std::make_shared<NumberKeyframeTrack>(name, times, values, interpolation); break;
    case Vector: track = std::make_shared<VectorKeyframeTrack>(name, times, values, interpolation); break;
    case Color: track = std::make_shared<ColorKeyframeTrack>(name, times, values, interpolation); break;
    case Quaternion: track = std::make_shared<QuaternionKeyframeTrack>(name, times, values, interpolation); break;
    case Boolean: track = std::make_shared<BooleanKeyframeTrack>(name, times, values, interpolation); break;
    case String: track = std::make_shared<StringKeyframeTrack>(name, times, values, interpolation); break;
    }
    if (hasTangents(AnimationSettings(json.get("settings")))) {
        const auto in = convertArray(json.get("settings").get("inTangents"), AnimationArrayType::Float32);
        const auto out = convertArray(json.get("settings").get("outTangents"), AnimationArrayType::Float32);
        track->settings = AnimationSettings(in, out);
    }
    return track;
}
}
AnimationClip::AnimationClip(JSValue n, double d, TracksPtr t, JSValue blend)
    : name(n.isUndefined() ? JSValue("") : std::move(n)), uuid(MathUtils::generateUUID()),
      blendMode(blend.isUndefined() ? JSValue(NormalAnimationBlendMode) : std::move(blend)),
      duration(d), tracks(std::move(t)) {
    if (duration < 0) resetDuration();
}
std::shared_ptr<AnimationClip> AnimationClip::parse(JSValue json) {
    auto tracks = std::make_shared<Tracks>();
    const auto jsonTracks = json.get("tracks"), fps = json.get("fps");
    const auto frameTime = 1. / (fps.toBoolean() ? fps.toNumber() : 1.);
    const auto n = jsonTracks.get("length").toNumber();
    for (double i = 0; i != n; ++i) {
        auto track = parseTrack(jsonTracks.get(JSValue(i).toString()));
        track->scale(frameTime); tracks->push_back(std::move(track));
    }
    const auto name = json.get("name"), duration = json.get("duration"), blend = json.get("blendMode");
    auto clip = std::make_shared<AnimationClip>(name, duration.isUndefined() ? -1 : duration.toNumber(), tracks, blend);
    clip->uuid = json.get("uuid");
    const auto user = json.get("userData");
    clip->userData = Animation_detail::parseJSON(user.toBoolean() ? user.toString() : "{}");
    return clip;
}
JSValue AnimationClip::toJSON(const AnimationClip& clip) {
    auto tracks = JSValue::array(); const auto source = clip.tracks;
    auto json = JSValue::object();
    json.set("name", clip.name); json.set("duration", clip.duration); json.set("tracks", tracks);
    json.set("uuid", clip.uuid); json.set("blendMode", clip.blendMode);
    const auto user = Animation_detail::stringifyJSON(clip.userData);
    json.set("userData", user ? JSValue(*user) : JSValue{});
    if (!source) throw JSTypeError("Cannot read properties of null tracks");
    for (std::size_t i = 0, n = source->size(); i != n; ++i)
        tracks.set(std::to_string(tracks.length()), KeyframeTrack::toJSON(*trackAt(source, i)));
    return json;
}
std::shared_ptr<AnimationClip> AnimationClip::CreateFromMorphTargetSequence(JSValue name, JSValue sequence,
                                                                          double fps, bool noLoop) {
    const auto n = sequence.get("length").toNumber();
    auto tracks = std::make_shared<Tracks>();
    for (double i = 0; i < n; ++i) {
        auto times = AnimationArray::array({std::fmod(i + n - 1, n), i, std::fmod(i + 1, n)});
        auto values = AnimationArray::array({0, 1, 0});
        const auto order = getKeyframeOrder(times);
        times = sortedArray(times, 1, order); values = sortedArray(values, 1, order);
        if (!noLoop && times.get(0).strictEquals(0)) {
            times.set(Animation_detail::length(times), n);
            values.set(Animation_detail::length(values), values.get(0));
        }
        const auto morphName = sequence.get(JSValue(i).toString()).get("name");
        const auto prefix = Interpolant_detail::add(".morphTargetInfluences[", morphName);
        const auto trackName = Interpolant_detail::add(prefix, "]");
        auto track = std::make_shared<NumberKeyframeTrack>(trackName, times, values);
        track->scale(1. / fps); tracks->push_back(std::move(track));
    }
    return std::make_shared<AnimationClip>(std::move(name), -1, tracks);
}
AnimationClip::Clips AnimationClip::CreateClipsFromMorphTargetSequences(JSValue targets, double fps, bool noLoop) {
    auto groups = JSValue::object();
    static const std::regex pattern(R"(^([\w-]*?)([\d]+)$)");
    for (double i = 0, n = targets.get("length").toNumber(); i < n; ++i) {
        const auto target = targets.get(JSValue(i).toString()), name = target.get("name");
        if (name.type() != JSValue::Type::String) throw JSTypeError("morphTarget.name.match is not a function");
        std::smatch parts; const auto text = name.string();
        if (std::regex_match(text, parts, pattern)) {
            const auto group = parts[1].str();
            static const std::vector<std::string> inherited{"constructor", "toString", "toLocaleString", "valueOf", "hasOwnProperty", "isPrototypeOf", "propertyIsEnumerable", "__defineGetter__", "__defineSetter__", "__lookupGetter__", "__lookupSetter__", "__proto__"};
            if (std::find(inherited.begin(), inherited.end(), group) != inherited.end())
                throw JSTypeError("animationMorphTargets.push is not a function");
            auto array = groups.get(group);
            if (!array.toBoolean()) { array = JSValue::array(); groups.set(group, array); }
            array.set(std::to_string(array.length()), target);
        }
    }
    Clips clips;
    for (const auto& name : groups.keys()) clips.push_back(CreateFromMorphTargetSequence(name, groups.get(name), fps, noLoop));
    return clips;
}
std::shared_ptr<AnimationClip> AnimationClip::findByName(const Clips& clips, const JSValue& name) {
    for (const auto& clip : clips) {
        if (!clip) throw JSTypeError("Cannot read properties of undefined clip");
        if (clip->name.strictEquals(name)) return clip;
    }
    return nullptr;
}
std::shared_ptr<AnimationClip> AnimationClip::findByName(const Object3D& object, const JSValue& name) {
    if (const auto mesh = dynamic_cast<const Mesh*>(&object); mesh && mesh->geometry && mesh->geometry->animations)
        return findByName(*mesh->geometry->animations, name);
    return findByName(object.animations, name);
}
JSValue AnimationClip::findByName(JSValue objectOrClipArray, JSValue name) {
    auto clips = objectOrClipArray;
    if (objectOrClipArray.type() != JSValue::Type::Array) {
        clips = objectOrClipArray.get("geometry");
        if (clips.toBoolean()) clips = objectOrClipArray.get("geometry").get("animations");
        if (!clips.toBoolean()) clips = objectOrClipArray.get("animations");
    }
    for (double i = 0; i < clips.get("length").toNumber(); ++i) {
        const auto key = JSValue(i).toString();
        if (clips.get(key).get("name").strictEquals(name)) return clips.get(key);
        if (i + 1 == i) throw JSRangeError("Clip search cannot progress");
    }
    return nullptr;
}
AnimationClip& AnimationClip::resetDuration() {
    const auto source = tracks; double result = 0;
    if (!source) throw JSTypeError("Cannot read properties of null tracks");
    for (std::size_t i = 0, n = source->size(); i != n; ++i) {
        const auto track = trackAt(tracks, i);
        const auto times = track->times;
        const auto last = Animation_detail::length(track->times) - 1;
        result = MathUtils::max(result, times.number(last));
    }
    duration = result; return *this;
}
AnimationClip& AnimationClip::trim() {
    for (std::size_t i = 0; i < tracks->size(); ++i) trackAt(tracks, i)->trim(0, duration);
    return *this;
}
bool AnimationClip::validate() const {
    bool valid = true;
    for (std::size_t i = 0; i < tracks->size(); ++i) valid = valid && trackAt(tracks, i)->validate();
    return valid;
}
AnimationClip& AnimationClip::optimize() {
    for (std::size_t i = 0; i < tracks->size(); ++i) trackAt(tracks, i)->optimize();
    return *this;
}
std::shared_ptr<AnimationClip> AnimationClip::constructClone(JSValue name, double duration,
                                                            TracksPtr tracks, JSValue blend) const {
    return std::make_shared<AnimationClip>(std::move(name), duration, std::move(tracks), std::move(blend));
}
std::shared_ptr<AnimationClip> AnimationClip::clone() const {
    auto result = std::make_shared<Tracks>();
    for (std::size_t i = 0; i < tracks->size(); ++i) result->push_back(trackAt(tracks, i)->clone());
    auto clip = constructClone(name, duration, result, blendMode);
    const auto json = Animation_detail::stringifyJSON(userData);
    clip->userData = Animation_detail::parseJSON(json.value_or("undefined"));
    return clip;
}

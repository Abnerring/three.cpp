// Direct port of three.js AnimationAction.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository LICENSE.
#include "animation/AnimationAction.h"
#include "math/Interpolant.h"
#include "constants.h"
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
using Interpolant_detail::add;
using Interpolant_detail::subtract;
using Interpolant_detail::multiply;
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue call(JSValue receiver, const char* name, Args args = {}) {
    const auto method = receiver.get(name);
    return method.call(receiver, args);
}
double divide(JSValue a, JSValue b) { const auto left = a.toNumber(); const auto right = b.toNumber(); return left / right; }
std::u16string utf16(const std::string& text) {
    std::u16string result;
    for (std::size_t i = 0; i < text.size();) {
        auto code = std::uint32_t(static_cast<unsigned char>(text[i++]));
        if (code >= 0xc0) {
            const auto count = code >= 0xf0 ? 3 : code >= 0xe0 ? 2 : 1;
            code &= count == 3 ? 7 : count == 2 ? 15 : 31;
            for (int j = 0; j < count && i < text.size(); ++j) code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        }
        if (code > 0xffff) { code -= 0x10000; result.push_back(char16_t(0xd800 + (code >> 10))); result.push_back(char16_t(0xdc00 + (code & 1023))); }
        else result.push_back(char16_t(code));
    }
    return result;
}
enum class Relation { Less, LessEqual, Greater, GreaterEqual };
bool compare(JSValue left, JSValue right, Relation relation) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number);
    const auto b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) {
        const auto x = utf16(a.string()), y = utf16(b.string());
        if (relation == Relation::Less) return x < y;
        if (relation == Relation::LessEqual) return x <= y;
        if (relation == Relation::Greater) return x > y;
        return x >= y;
    }
    const auto x = a.toNumber(), y = b.toNumber();
    if (relation == Relation::Less) return x < y;
    if (relation == Relation::LessEqual) return x <= y;
    if (relation == Relation::Greater) return x > y;
    return x >= y;
}
bool less(JSValue a, JSValue b) { return compare(a, b, Relation::Less); }
bool greater(JSValue a, JSValue b) { return compare(a, b, Relation::Greater); }
bool ge(JSValue a, JSValue b) { return compare(a, b, Relation::GreaterEqual); }
bool odd(JSValue value) {
    const auto number = value.toNumber();
    return std::isfinite(number) && std::fmod(std::abs(std::trunc(number)), 2) == 1;
}
JSValue newArray(JSValue length) {
    if (length.type() != JSValue::Type::Number) { auto array = JSValue::array(1); array.set("0", length); return array; }
    const auto n = length.number();
    if (!std::isfinite(n) || n < 0 || n > UINT32_MAX || n != std::trunc(n)) throw JSRangeError("Invalid array length");
    return JSValue::array(static_cast<std::uint32_t>(n));
}
void progress(double index) {
    if (index + 1 == index) throw JSRangeError("AnimationAction loop cannot progress");
}
void emit(JSValue self, const char* type, const char* key, JSValue value) {
    auto mixer = self.get("_mixer"); const auto method = mixer.get("dispatchEvent");
    auto event = JSValue::object(); event.set("type", type); event.set("action", self); event.set(key, value);
    method.call(mixer, {event});
}

JSValue method(const JSValue& receiver, const Args& args, const Args& capture) {
    auto self = receiver;
    const auto& name = capture[0].string();
    if (name == "play") { call(self.get("_mixer"), "_activateAction", {self}); return self; }
    if (name == "stop") { call(self.get("_mixer"), "_deactivateAction", {self}); return call(self, "reset"); }
    if (name == "reset") {
        self.set("paused", false); self.set("enabled", true); self.set("time", 0); self.set("_loopCount", -1); self.set("_startTime", nullptr);
        return call(call(self, "stopFading"), "stopWarping");
    }
    if (name == "isRunning") {
        const auto enabled = self.get("enabled"); if (!enabled.toBoolean()) return enabled;
        if (self.get("paused").toBoolean()) return false;
        if (self.get("timeScale").strictEquals(0)) return false;
        if (!self.get("_startTime").isNull()) return false;
        return call(self.get("_mixer"), "_isActiveAction", {self});
    }
    if (name == "isScheduled") return call(self.get("_mixer"), "_isActiveAction", {self});
    if (name == "startAt") { self.set("_startTime", arg(args, 0)); return self; }
    if (name == "setLoop") { self.set("loop", arg(args, 0)); self.set("repetitions", arg(args, 1)); return self; }
    if (name == "setEffectiveWeight") {
        const auto weight = arg(args, 0); self.set("weight", weight);
        self.set("_effectiveWeight", self.get("enabled").toBoolean() ? weight : JSValue(0)); return call(self, "stopFading");
    }
    if (name == "getEffectiveWeight") return self.get("_effectiveWeight");
    if (name == "fadeIn") return call(self, "_scheduleFading", {arg(args, 0), 0, 1});
    if (name == "fadeOut") return call(self, "_scheduleFading", {arg(args, 0), 1, 0});
    if (name == "crossFadeFrom") {
        auto other = arg(args, 0); const auto duration = arg(args, 1);
        call(other, "fadeOut", {duration}); call(self, "fadeIn", {duration});
        if (arg(args, 2).strictEquals(true)) {
            const auto inDuration = self.get("_clip").get("duration"), outDuration = other.get("_clip").get("duration");
            const auto startEnd = divide(outDuration, inDuration), endStart = divide(inDuration, outDuration);
            other.set("_restoreTimeScale", other.get("timeScale")); self.set("_restoreTimeScale", self.get("timeScale"));
            call(other, "warp", {1., startEnd, duration}); call(self, "warp", {endStart, 1., duration});
        }
        return self;
    }
    if (name == "crossFadeTo") { auto warp = arg(args, 2); if (warp.isUndefined()) warp = false; return call(arg(args, 0), "crossFadeFrom", {self, arg(args, 1), warp}); }
    if (name == "stopFading" || name == "stopWarping") {
        const char* field = name == "stopFading" ? "_weightInterpolant" : "_timeScaleInterpolant";
        const auto interpolant = self.get(field);
        if (!interpolant.isNull()) { self.set(field, nullptr); call(self.get("_mixer"), "_takeBackControlInterpolant", {interpolant}); }
        if (name == "stopWarping") self.set("_restoreTimeScale", nullptr);
        return self;
    }
    if (name == "setEffectiveTimeScale") {
        const auto scale = arg(args, 0); self.set("timeScale", scale);
        self.set("_effectiveTimeScale", self.get("paused").toBoolean() ? JSValue(0) : scale); return call(self, "stopWarping");
    }
    if (name == "getEffectiveTimeScale") return self.get("_effectiveTimeScale");
    if (name == "setDuration") { const auto duration = self.get("_clip").get("duration"); self.set("timeScale", divide(duration, arg(args, 0))); return call(self, "stopWarping"); }
    if (name == "syncWith") { const auto other = arg(args, 0); self.set("time", other.get("time")); self.set("timeScale", other.get("timeScale")); return call(self, "stopWarping"); }
    if (name == "halt") { const auto callback = self.get("warp"); const auto scale = self.get("_effectiveTimeScale"); return callback.call(self, {scale, 0, arg(args, 0)}); }
    if (name == "warp" || name == "_scheduleFading") {
        const bool warping = name == "warp";
        const auto mixer = self.get("_mixer"), now = mixer.get("time");
        const auto scale = warping ? self.get("timeScale") : JSValue{};
        const char* field = warping ? "_timeScaleInterpolant" : "_weightInterpolant";
        auto interpolant = self.get(field);
        if (interpolant.isNull()) { interpolant = call(mixer, "_lendControlInterpolant"); self.set(field, interpolant); }
        auto times = interpolant.get("parameterPositions"), values = interpolant.get("sampleValues");
        times.set("0", now);
        if (warping) {
            times.set("1", add(now, arg(args, 2)));
            values.set("0", divide(arg(args, 0), scale)); values.set("1", divide(arg(args, 1), scale));
        } else {
            values.set("0", arg(args, 1)); times.set("1", add(now, arg(args, 0))); values.set("1", arg(args, 2));
        }
        return self;
    }
    if (name == "getMixer") return self.get("_mixer");
    if (name == "getClip") return self.get("_clip");
    if (name == "getRoot") { const auto local = self.get("_localRoot"); return local.toBoolean() ? local : self.get("_mixer").get("_root"); }
    if (name == "_update") {
        const auto time = arg(args, 0), direction = arg(args, 2), accuIndex = arg(args, 3); auto delta = arg(args, 1);
        if (!self.get("enabled").toBoolean()) { call(self, "_updateWeight", {time}); return {}; }
        const auto start = self.get("_startTime");
        if (!start.isNull()) {
            const auto running = multiply(subtract(time, start), direction);
            if (running < 0 || direction.strictEquals(0)) delta = 0;
            else { self.set("_startTime", nullptr); delta = multiply(direction, running); }
        }
        const auto scale = call(self, "_updateTimeScale", {time}); delta = multiply(delta, scale);
        const auto clipTime = call(self, "_updateTime", {delta});
        const auto weight = call(self, "_updateWeight", {time});
        if (greater(weight, 0)) {
            const auto interpolants = self.get("_interpolants"), mixers = self.get("_propertyBindings");
            const bool additive = self.get("blendMode").strictEquals(AdditiveAnimationBlendMode);
            const auto count = interpolants.get("length");
            for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
                const auto key = JSValue(i).toString(); call(interpolants.get(key), "evaluate", {clipTime});
                if (additive) call(mixers.get(key), "accumulateAdditive", {weight});
                else call(mixers.get(key), "accumulate", {accuIndex, weight});
                progress(i);
            }
        }
        return {};
    }
    if (name == "_updateWeight") {
        const auto time = arg(args, 0); JSValue weight = 0;
        if (self.get("enabled").toBoolean()) {
            weight = self.get("weight"); const auto interpolant = self.get("_weightInterpolant");
            if (!interpolant.isNull()) {
                const auto value = call(interpolant, "evaluate", {time}).get("0"); weight = multiply(weight, value);
                const auto end = interpolant.get("parameterPositions").get("1");
                if (greater(time, end)) { call(self, "stopFading"); if (value.strictEquals(0)) self.set("enabled", false); }
            }
        }
        self.set("_effectiveWeight", weight); return weight;
    }
    if (name == "_updateTimeScale") {
        const auto time = arg(args, 0); JSValue scale = 0;
        if (!self.get("paused").toBoolean()) {
            scale = self.get("timeScale"); const auto interpolant = self.get("_timeScaleInterpolant");
            if (!interpolant.isNull()) {
                const auto value = call(interpolant, "evaluate", {time}).get("0"); scale = multiply(scale, value);
                const auto end = interpolant.get("parameterPositions").get("1");
                if (greater(time, end)) {
                    if (scale.strictEquals(0)) self.set("paused", true);
                    else { if (!self.get("_restoreTimeScale").isNull()) scale = self.get("_restoreTimeScale"); self.set("timeScale", scale); }
                    call(self, "stopWarping");
                }
            }
        }
        self.set("_effectiveTimeScale", scale); return scale;
    }
    if (name == "_updateTime") {
        const auto delta = arg(args, 0), duration = self.get("_clip").get("duration"), loop = self.get("loop");
        auto time = add(self.get("time"), delta), loopCount = self.get("_loopCount");
        const bool pingPong = loop.strictEquals(LoopPingPong);
        if (delta.strictEquals(0)) {
            if (loopCount.strictEquals(-1)) return time;
            return pingPong && odd(loopCount) ? JSValue(subtract(duration, time)) : time;
        }
        if (loop.strictEquals(LoopOnce)) {
            if (loopCount.strictEquals(-1)) { self.set("_loopCount", 0); call(self, "_setEndings", {true, true, false}); }
            bool stop = true;
            if (ge(time, duration)) time = duration;
            else if (less(time, 0)) time = 0;
            else { self.set("time", time); stop = false; }
            if (stop) {
                if (self.get("clampWhenFinished").toBoolean()) self.set("paused", true); else self.set("enabled", false);
                self.set("time", time);
                const auto mixer = self.get("_mixer"), callback = mixer.get("dispatchEvent");
                auto event = JSValue::object(); event.set("type", "finished"); event.set("action", self); event.set("direction", less(delta, 0) ? -1 : 1);
                callback.call(mixer, {event});
            }
        } else {
            if (loopCount.strictEquals(-1)) {
                if (ge(delta, 0)) {
                    loopCount = 0; const auto callback = self.get("_setEndings"); const auto last = self.get("repetitions").strictEquals(0);
                    callback.call(self, {true, last, pingPong});
                } else {
                    const auto callback = self.get("_setEndings"); const auto first = self.get("repetitions").strictEquals(0);
                    callback.call(self, {first, true, pingPong});
                }
            }
            if (ge(time, duration) || less(time, 0)) {
                const auto loopDelta = std::floor(divide(time, duration));
                const auto offset = multiply(duration, loopDelta); time = subtract(time, offset);
                loopCount = add(loopCount, std::abs(loopDelta));
                const auto pending = subtract(self.get("repetitions"), loopCount);
                if (pending <= 0) {
                    if (self.get("clampWhenFinished").toBoolean()) self.set("paused", true); else self.set("enabled", false);
                    time = greater(delta, 0) ? duration : JSValue(0); self.set("time", time);
                    const auto mixer = self.get("_mixer"), callback = mixer.get("dispatchEvent");
                    auto event = JSValue::object(); event.set("type", "finished"); event.set("action", self); event.set("direction", greater(delta, 0) ? 1 : -1);
                    callback.call(mixer, {event});
                } else {
                    if (pending == 1) { const bool start = less(delta, 0); call(self, "_setEndings", {start, !start, pingPong}); }
                    else call(self, "_setEndings", {false, false, pingPong});
                    self.set("_loopCount", loopCount); self.set("time", time); emit(self, "loop", "loopDelta", loopDelta);
                }
            } else { self.set("_loopCount", loopCount); self.set("time", time); }
            if (pingPong && odd(loopCount)) return subtract(duration, time);
        }
        return time;
    }
    if (name == "_setEndings") {
        auto settings = self.get("_interpolantSettings");
        if (arg(args, 2).toBoolean()) { settings.set("endingStart", ZeroSlopeEnding); settings.set("endingEnd", ZeroSlopeEnding); }
        else {
            settings.set("endingStart", arg(args, 0).toBoolean() ? (self.get("zeroSlopeAtStart").toBoolean() ? ZeroSlopeEnding : ZeroCurvatureEnding) : WrapAroundEnding);
            settings.set("endingEnd", arg(args, 1).toBoolean() ? (self.get("zeroSlopeAtEnd").toBoolean() ? ZeroSlopeEnding : ZeroCurvatureEnding) : WrapAroundEnding);
        }
        return {};
    }
    throw JSTypeError("Unknown AnimationAction method");
}
}

JSValue AnimationAction::prototype() {
    static thread_local auto value = [] {
        auto result = JSValue::object();
        auto constructor = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSTypeError("Class constructor AnimationAction cannot be invoked without 'new'");
        }, {}, "AnimationAction", 2);
        JSPropertyDescriptor prototypeProperty; prototypeProperty.value = result;
        prototypeProperty.writable = prototypeProperty.enumerable = prototypeProperty.configurable = false;
        constructor.defineProperty("prototype", prototypeProperty);
        JSPropertyDescriptor constructorProperty; constructorProperty.value = constructor;
        constructorProperty.writable = constructorProperty.configurable = true; constructorProperty.enumerable = false;
        result.defineProperty("constructor", constructorProperty);
        const std::pair<const char*, unsigned> methods[]{
            {"play",0},{"stop",0},{"reset",0},{"isRunning",0},{"isScheduled",0},{"startAt",1},{"setLoop",2},
            {"setEffectiveWeight",1},{"getEffectiveWeight",0},{"fadeIn",1},{"fadeOut",1},{"crossFadeFrom",2},{"crossFadeTo",2},
            {"stopFading",0},{"setEffectiveTimeScale",1},{"getEffectiveTimeScale",0},{"setDuration",1},{"syncWith",1},
            {"halt",1},{"warp",3},{"stopWarping",0},{"getMixer",0},{"getClip",0},{"getRoot",0},
            {"_update",4},{"_updateWeight",1},{"_updateTimeScale",1},{"_updateTime",1},{"_setEndings",3},{"_scheduleFading",3}};
        for (const auto& [name, arity] : methods) {
            JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(method, {name}, name, arity);
            descriptor.writable = descriptor.configurable = true; result.defineProperty(name, descriptor);
        }
        return result;
    }();
    return value;
}
AnimationAction::AnimationAction(JSValue mixer, JSValue clip, JSValue localRoot, JSValue blendMode)
    : object_(JSValue::object(prototype())) {
    if (localRoot.isUndefined()) localRoot = nullptr;
    if (blendMode.isUndefined()) blendMode = clip.get("blendMode");
    object_.set("_mixer", mixer); object_.set("_clip", clip); object_.set("_localRoot", localRoot); object_.set("blendMode", blendMode);
    const auto tracks = clip.get("tracks"), count = tracks.get("length"); auto interpolants = newArray(count), settings = JSValue::object();
    settings.set("endingStart", ZeroCurvatureEnding); settings.set("endingEnd", ZeroCurvatureEnding);
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto key = JSValue(i).toString(); auto interpolant = call(tracks.get(key), "createInterpolant", {nullptr});
        interpolants.set(key, interpolant); interpolant.set("settings", settings); progress(i);
    }
    object_.set("_interpolantSettings", settings); object_.set("_interpolants", interpolants); object_.set("_propertyBindings", newArray(count));
    object_.set("_cacheIndex", nullptr); object_.set("_byClipCacheIndex", nullptr); object_.set("_timeScaleInterpolant", nullptr);
    object_.set("_restoreTimeScale", nullptr); object_.set("_weightInterpolant", nullptr);
    object_.set("loop", LoopRepeat); object_.set("_loopCount", -1); object_.set("_startTime", nullptr);
    object_.set("time", 0); object_.set("timeScale", 1); object_.set("_effectiveTimeScale", 1);
    object_.set("weight", 1); object_.set("_effectiveWeight", 1); object_.set("repetitions", std::numeric_limits<double>::infinity());
    object_.set("paused", false); object_.set("enabled", true); object_.set("clampWhenFinished", false);
    object_.set("zeroSlopeAtStart", true); object_.set("zeroSlopeAtEnd", true);
}
AnimationAction AnimationAction::fromObject(JSValue object) { return AnimationAction(std::move(object), Existing{}); }

#define ACTION0(name) JSValue AnimationAction::name() { return call(object_, #name); }
#define ACTION1(name, a) JSValue AnimationAction::name(JSValue a) { return call(object_, #name, {a}); }
#define ACTION2(name, a, b) JSValue AnimationAction::name(JSValue a, JSValue b) { return call(object_, #name, {a,b}); }
#define ACTION3(name, a, b, c) JSValue AnimationAction::name(JSValue a, JSValue b, JSValue c) { return call(object_, #name, {a,b,c}); }
ACTION0(play) ACTION0(stop) ACTION0(reset) ACTION0(isRunning) ACTION0(isScheduled)
ACTION1(startAt, time) ACTION2(setLoop, mode, repetitions) ACTION1(setEffectiveWeight, weight) ACTION0(getEffectiveWeight)
ACTION1(fadeIn, duration) ACTION1(fadeOut, duration) ACTION3(crossFadeFrom, action, duration, warp) ACTION3(crossFadeTo, action, duration, warp)
ACTION0(stopFading) ACTION1(setEffectiveTimeScale, timeScale) ACTION0(getEffectiveTimeScale) ACTION1(setDuration, duration)
ACTION1(syncWith, action) ACTION1(halt, duration) ACTION3(warp, startTimeScale, endTimeScale, duration) ACTION0(stopWarping)
ACTION0(getMixer) ACTION0(getClip) ACTION0(getRoot) ACTION1(_updateWeight, time) ACTION1(_updateTimeScale, time)
ACTION1(_updateTime, deltaTime) ACTION3(_setEndings, atStart, atEnd, pingPong) ACTION3(_scheduleFading, duration, weightNow, weightThen)
JSValue AnimationAction::_update(JSValue time, JSValue delta, JSValue direction, JSValue accu) { return call(object_, "_update", {time,delta,direction,accu}); }
#undef ACTION0
#undef ACTION1
#undef ACTION2
#undef ACTION3

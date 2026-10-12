#include "animation/KeyframeTrack.h"
#include "math/interpolants/DiscreteInterpolant.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "math/interpolants/BezierInterpolant.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include <cmath>
#include <iostream>

std::shared_ptr<Interpolant> InterpolantFactory::operator()(AnimationArray result) const {
    const auto function = function_;
    if (!function) throw JSTypeError("track.createInterpolant is not a function");
    return function->callback(*owner_, std::move(result));
}
KeyframeTrack::KeyframeTrack(JSValue n, Array t, Array v, JSValue interpolation)
    : KeyframeTrack(std::move(n), std::move(t), std::move(v), std::move(interpolation),
                    {"", AnimationArrayType::Float32, InterpolateLinear, true, true}) {}
KeyframeTrack::KeyframeTrack(JSValue n, Array t, Array v, JSValue interpolation, Profile profile)
    : name(std::move(n)), ValueTypeName(std::move(profile.type)), ValueBufferType(profile.values),
      DefaultInterpolation(profile.interpolation), linear_(profile.linear), smooth_(profile.smooth),
      quaternionLinear_(ValueTypeName == "quaternion") {
    if (name.isUndefined()) throw std::runtime_error("THREE.KeyframeTrack: track name is undefined");
    if (t.isUndefined() || Animation_detail::length(t) == 0)
        throw std::runtime_error("THREE.KeyframeTrack: no keyframes in track named " + name.toString());
    times = convertArray(std::move(t), TimeBufferType);
    values = convertArray(std::move(v), ValueBufferType);
    setInterpolation(interpolation.toBoolean() ? interpolation : DefaultInterpolation);
}
JSValue KeyframeTrack::toJSON(const KeyframeTrack& track) {
    auto json = JSValue::object();
    json.set("name", track.name);
    json.set("times", Animation_detail::ordinary(track.times));
    json.set("values", Animation_detail::ordinary(track.values));
    const auto interpolation = track.getInterpolation();
    if (!interpolation.strictEquals(track.DefaultInterpolation)) json.set("interpolation", interpolation);
    if (hasTangents(track.settings)) {
        auto settings = JSValue::object();
        settings.set("inTangents", Animation_detail::ordinary(track.settings.get("inTangents")));
        settings.set("outTangents", Animation_detail::ordinary(track.settings.get("outTangents")));
        json.set("settings", settings);
    }
    json.set("type", track.ValueTypeName);
    return json;
}
std::shared_ptr<Interpolant> KeyframeTrack::InterpolantFactoryMethodDiscrete(Array result) {
    const auto t = times, v = values; const auto size = getValueSize();
    return std::make_shared<DiscreteInterpolant>(t, v, size, std::move(result));
}
std::shared_ptr<Interpolant> KeyframeTrack::InterpolantFactoryMethodLinear(Array result) {
    const auto t = times, v = values; const auto size = getValueSize();
    return std::make_shared<LinearInterpolant>(t, v, size, std::move(result));
}
std::shared_ptr<Interpolant> KeyframeTrack::InterpolantFactoryMethodSmooth(Array result) {
    const auto t = times, v = values; const auto size = getValueSize();
    return std::make_shared<CubicInterpolant>(t, v, size, std::move(result));
}
std::shared_ptr<Interpolant> KeyframeTrack::InterpolantFactoryMethodBezier(Array result) {
    const auto t = times, v = values; const auto size = getValueSize();
    auto interpolant = std::make_shared<BezierInterpolant>(t, v, size, std::move(result));
    if (settings.toBoolean()) {
        interpolant->inTangents = settings.get("inTangents");
        interpolant->outTangents = settings.get("outTangents");
    }
    return interpolant;
}
InterpolantFactory::FunctionPtr KeyframeTrack::builtinFactory(int mode) const {
    static const auto discrete = InterpolantFactory::custom([](KeyframeTrack& t, Array r) { return t.KeyframeTrack::InterpolantFactoryMethodDiscrete(r); });
    static const auto linear = InterpolantFactory::custom([](KeyframeTrack& t, Array r) { return t.KeyframeTrack::InterpolantFactoryMethodLinear(r); });
    static const auto smooth = InterpolantFactory::custom([](KeyframeTrack& t, Array r) { return t.KeyframeTrack::InterpolantFactoryMethodSmooth(r); });
    static const auto bezier = InterpolantFactory::custom([](KeyframeTrack& t, Array r) { return t.KeyframeTrack::InterpolantFactoryMethodBezier(r); });
    static const auto quaternion = InterpolantFactory::custom([](KeyframeTrack& t, Array r) {
        const auto times = t.times, values = t.values; const auto size = t.getValueSize();
        return std::make_shared<QuaternionLinearInterpolant>(times, values, size, r);
    });
    switch (mode) {
    case InterpolateDiscrete: return discrete;
    case InterpolateLinear: return quaternionLinear_ ? quaternion : linear;
    case InterpolateSmooth: return smooth;
    case InterpolateBezier: return bezier;
    default: return {};
    }
}
bool KeyframeTrack::supportsInterpolation(int mode) const {
    return mode == InterpolateDiscrete || mode == InterpolateBezier ||
           (mode == InterpolateLinear && linear_) || (mode == InterpolateSmooth && smooth_);
}
KeyframeTrack& KeyframeTrack::setInterpolation(JSValue interpolation) {
    InterpolantFactory::FunctionPtr factory;
    for (const int mode : {InterpolateDiscrete, InterpolateLinear, InterpolateSmooth, InterpolateBezier})
        if (interpolation.strictEquals(mode) && supportsInterpolation(mode)) factory = builtinFactory(mode);
    if (!factory) {
        const auto message = "unsupported interpolation for " + ValueTypeName + " keyframe track named " + name.toString();
        if (!createInterpolant.function()) {
            if (!interpolation.strictEquals(DefaultInterpolation)) setInterpolation(DefaultInterpolation);
            else throw std::runtime_error(message);
        }
        std::cerr << "THREE.KeyframeTrack: " << message << '\n';
        return *this;
    }
    createInterpolant = factory;
    return *this;
}
JSValue KeyframeTrack::getInterpolation() const {
    for (const int mode : {InterpolateDiscrete, InterpolateLinear, InterpolateSmooth, InterpolateBezier})
        if (createInterpolant.function() == (supportsInterpolation(mode) ? builtinFactory(mode) : nullptr)) return mode;
    return {};
}
double KeyframeTrack::getValueSize() const {
    const auto valueCount = Animation_detail::lengthValue(values);
    const auto timeCount = Animation_detail::lengthValue(times);
    const auto a = valueCount.toNumber(), b = timeCount.toNumber();
    return a / b;
}
KeyframeTrack& KeyframeTrack::shift(JSValue offset) {
    if (!offset.strictEquals(0)) {
        auto t = times; const auto n = Animation_detail::length(t);
        for (double i = 0; i != n; ++i) t.set(i, Interpolant_detail::add(t.get(i), offset));
    }
    return *this;
}
KeyframeTrack& KeyframeTrack::scale(JSValue scale) {
    if (!scale.strictEquals(1)) {
        auto t = times; const auto n = Animation_detail::length(t);
        for (double i = 0; i != n; ++i) t.set(i, Interpolant_detail::multiply(t.get(i), scale));
        if (hasTangents(settings)) {
            const auto scaleTangents = [&](Array tangent) {
                const auto n = Animation_detail::length(tangent);
                if (!std::isfinite(n) || n < 0 || std::fmod(n, 2) != 0)
                    throw JSRangeError("Odd tangent length would cause a nonterminating loop");
                for (double i = 0; i != n; i += 2)
                    tangent.set(i, Interpolant_detail::multiply(tangent.get(i), scale));
            };
            scaleTangents(settings.get("inTangents"));
            scaleTangents(settings.get("outTangents"));
        }
    }
    return *this;
}
KeyframeTrack& KeyframeTrack::trim(double start, double end) {
    const auto t = times; const auto n = Animation_detail::length(t);
    double from = 0, to = n - 1;
    while (from != n && t.number(from) < start) ++from;
    while (to != -1 && t.number(to) > end) --to;
    ++to;
    if (from != 0 || to != n) {
        if (from >= to) { to = std::max(to, 1.); from = to - 1; }
        const auto stride = getValueSize();
        times = Animation_detail::slice(t, from, to);
        values = Animation_detail::slice(values, from * stride, to * stride);
    }
    return *this;
}
bool KeyframeTrack::validate() const {
    bool valid = true;
    const auto report = [&](const char* message) { std::cerr << "THREE.KeyframeTrack: " << message << '\n'; valid = false; };
    const auto stride = getValueSize();
    if (stride - std::floor(stride) != 0) report("Invalid value size in track.");
    const auto t = times, v = values; const auto n = Animation_detail::length(t);
    if (n == 0) report("Track is empty.");
    JSValue previous = nullptr;
    for (double i = 0; i != n; ++i) {
        const auto current = t.get(i);
        if (current.type() == JSValue::Type::Number && std::isnan(current.number())) { report("Time is not a valid number."); break; }
        if (!previous.isNull()) {
            const auto a = previous.toNumber(), b = current.toNumber();
            if (a > b) { report("Out of order keys."); break; }
        }
        previous = current;
    }
    if (!v.isUndefined() && isTypedArray(v))
        for (double i = 0, n = Animation_detail::length(v); i != n; ++i)
            if (std::isnan(v.number(i))) { report("Value is not a valid number."); break; }
    return valid;
}
KeyframeTrack& KeyframeTrack::optimize() {
    auto t = Animation_detail::slice(times), v = Animation_detail::slice(values);
    const auto stride = getValueSize();
    const bool smooth = getInterpolation().strictEquals(InterpolateSmooth);
    const auto last = Animation_detail::length(t) - 1;
    double write = 1;
    for (double i = 1; i < last; ++i) {
        bool keep = false;
        const auto time = t.get(i), next = t.get(i + 1);
        if (!time.strictEquals(next) && (i != 1 || !time.strictEquals(t.get(0)))) {
            if (!smooth) {
                Animation_detail::requireStride(stride);
                const auto offset = i * stride, prev = offset - stride, next = offset + stride;
                for (double j = 0; j != stride; ++j) {
                    const auto value = v.get(offset + j);
                    if (!value.strictEquals(v.get(prev + j)) || !value.strictEquals(v.get(next + j))) { keep = true; break; }
                }
            } else keep = true;
        }
        if (keep) {
            if (i != write) {
                t.set(write, t.get(i));
                Animation_detail::requireStride(stride);
                const auto readOffset = i * stride, writeOffset = write * stride;
                for (double j = 0; j != stride; ++j) v.set(writeOffset + j, v.get(readOffset + j));
            }
            ++write;
        }
    }
    if (last > 0) {
        t.set(write, t.get(last));
        Animation_detail::requireStride(stride);
        const auto readOffset = last * stride, writeOffset = write * stride;
        for (double j = 0; j != stride; ++j) v.set(writeOffset + j, v.get(readOffset + j));
        ++write;
    }
    if (write != Animation_detail::length(t)) {
        times = Animation_detail::slice(t, 0, write);
        values = Animation_detail::slice(v, 0, write * stride);
    } else { times = t; values = v; }
    return *this;
}
std::shared_ptr<KeyframeTrack> KeyframeTrack::constructClone(JSValue n, Array t, Array v) const {
    return std::make_shared<KeyframeTrack>(std::move(n), std::move(t), std::move(v));
}
std::shared_ptr<KeyframeTrack> KeyframeTrack::clone() const {
    const auto t = Animation_detail::slice(times), v = Animation_detail::slice(values);
    auto track = constructClone(name, t, v);
    track->createInterpolant = createInterpolant;
    if (hasTangents(settings)) {
        const auto in = Animation_detail::slice(settings.get("inTangents"));
        const auto out = Animation_detail::slice(settings.get("outTangents"));
        track->settings = AnimationSettings(in, out);
    }
    return track;
}

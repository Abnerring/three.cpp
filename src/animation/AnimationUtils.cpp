#include "animation/AnimationUtils.h"
#include "animation/AnimationClip.h"
#include "core/ArraySort.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {
AnimationArray allocate(AnimationArrayType type, double length) {
    switch (type) {
    case AnimationArrayType::Int8: return Int8Array(length);
    case AnimationArrayType::Uint8: return Uint8Array(length);
    case AnimationArrayType::Int16: return Int16Array(length);
    case AnimationArrayType::Uint16: return Uint16Array(length);
    case AnimationArrayType::Int32: return Int32Array(length);
    case AnimationArrayType::Uint32: return Uint32Array(length);
    case AnimationArrayType::Float32: return Float32Array(length);
    case AnimationArrayType::Float64: return Float64Array(length);
    case AnimationArrayType::Uint8Clamped: return Uint8ClampedArray(length);
    default:
        if (!std::isfinite(length) || length < 0 || length > UINT32_MAX || std::trunc(length) != length)
            throw JSRangeError("Invalid array length");
        return JSValue::array(static_cast<std::uint32_t>(length));
    }
}
double toLength(double length) {
    if (std::isnan(length) || length <= 0) return 0;
    return std::min(std::floor(length), 9007199254740991.);
}
double relativeIndex(double index, double length) {
    if (std::isnan(index)) return 0;
    index = std::trunc(index);
    return index < 0 ? std::max(length + index, 0.) : std::min(index, length);
}
void push(JSValue array, const JSValue& value) { array.set(std::to_string(array.length()), value); }
std::shared_ptr<KeyframeTrack> trackAt(const AnimationClip::TracksPtr& tracks, std::size_t index) {
    if (!tracks || index >= tracks->size() || !(*tracks)[index]) throw JSTypeError("Cannot read properties of undefined track");
    return (*tracks)[index];
}
}
bool AnimationSettings::isUndefined() const {
    const auto value = std::get_if<JSValue>(&value_);
    return value && value->isUndefined();
}
bool AnimationSettings::toBoolean() const {
    const auto value = std::get_if<JSValue>(&value_);
    return !value || value->toBoolean();
}
bool AnimationSettings::sameIdentity(const AnimationSettings& other) const {
    if (value_.index() != other.value_.index()) return false;
    if (const auto value = std::get_if<JSValue>(&value_)) return value->sameValue(std::get<JSValue>(other.value_));
    return std::get<std::shared_ptr<Tangents>>(value_) == std::get<std::shared_ptr<Tangents>>(other.value_);
}
AnimationArray AnimationSettings::get(const std::string& key) const {
    if (const auto value = std::get_if<JSValue>(&value_)) return value->get(key);
    const auto tangents = std::get<std::shared_ptr<Tangents>>(value_);
    if (key == "inTangents") return tangents->in;
    if (key == "outTangents") return tangents->out;
    return {};
}
void AnimationSettings::set(const std::string& key, AnimationArray value) {
    if (auto ordinary = std::get_if<JSValue>(&value_)) {
        if (const auto raw = std::get_if<JSValue>(&value.storage())) { ordinary->set(key, *raw); return; }
        throw JSTypeError("Use AnimationSettings(in, out) for typed tangent properties");
    }
    const auto tangents = std::get<std::shared_ptr<Tangents>>(value_);
    if (key == "inTangents") tangents->in = std::move(value);
    else if (key == "outTangents") tangents->out = std::move(value);
    else throw JSTypeError("Typed tangent settings only contain inTangents and outTangents");
}
bool isTypedArray(const AnimationArray& array) { return !std::holds_alternative<JSValue>(array.storage()); }
JSValue Animation_detail::lengthValue(const AnimationArray& array) {
    if (const auto value = std::get_if<JSValue>(&array.storage())) return value->get("length");
    return array.size();
}
double Animation_detail::length(const AnimationArray& array) { return lengthValue(array).toNumber(); }
AnimationArray convertArray(AnimationArray array, AnimationArrayType type) {
    if (!array.toBoolean()) return array;
    const auto ordinary = std::get_if<JSValue>(&array.storage());
    if (std::size_t(type) == array.storage().index() && (!ordinary || ordinary->type() == JSValue::Type::Array)) return array;
    if (ordinary && !ordinary->isObject() && type != AnimationArrayType::Array)
        return allocate(type, ordinary->toNumber());
    if (ordinary && ordinary->type() == JSValue::Type::Array && type != AnimationArrayType::Array) {
        // TypedArray construction consumes the array iterator before coercing
        // its values. Accessors can grow the array during that iteration.
        std::vector<JSValue> elements;
        for (double i = 0; i < ordinary->get("length").toNumber(); ++i) elements.push_back(array.get(i));
        auto result = allocate(type, double(elements.size()));
        for (std::size_t i = 0; i < elements.size(); ++i) result.set(double(i), elements[i]);
        return result;
    }
    const auto n = toLength(Animation_detail::length(array));
    auto result = allocate(type, n);
    for (double i = 0; i < n; ++i) {
        if (type == AnimationArrayType::Array && ordinary) {
            const auto key = JSValue(i).toString();
            if (!(ordinary->isObject() ? ordinary->has(key) : ordinary->hasOwn(key))) continue;
        }
        result.set(i, array.get(i));
    }
    return result;
}
JSValue Animation_detail::ordinary(const AnimationArray& array) {
    const auto result = convertArray(array, AnimationArrayType::Array);
    return std::get<JSValue>(result.storage());
}
AnimationArray Animation_detail::slice(AnimationArray array, double begin, std::optional<double> end) {
    return std::visit([&](const auto& values) -> AnimationArray {
        using T = std::decay_t<decltype(values)>;
        if constexpr (!std::is_same_v<T, JSValue>) return values.slice(begin, end);
        else {
            const auto n = toLength(length(array));
            const auto first = relativeIndex(begin, n), last = end ? relativeIndex(*end, n) : n;
            auto result = allocate(AnimationArrayType::Array, std::max(last - first, 0.));
            for (double i = first; i < last; ++i) {
                const auto key = JSValue(i).toString();
                if (values.isObject() ? values.has(key) : values.hasOwn(key)) result.set(i - first, values.get(key));
            }
            return result;
        }
    }, array.storage());
}
void Animation_detail::requireStride(double stride) {
    if (!std::isfinite(stride) || stride < 0 || std::trunc(stride) != stride || stride > 9007199254740991.)
        throw JSRangeError("Animation stride would cause a nonterminating loop");
}
bool hasTangents(AnimationSettings settings) {
    return !settings.isUndefined() && !settings.get("inTangents").isUndefined() && !settings.get("outTangents").isUndefined();
}
AnimationArray getKeyframeOrder(AnimationArray times) {
    const auto n = Animation_detail::length(times);
    auto result = allocate(AnimationArrayType::Array, n);
    std::vector<double> order;
    order.reserve(static_cast<std::size_t>(n));
    for (double i = 0; i != n; ++i) order.push_back(i);
    v8ArraySort(order, [&](double i, double j) {
        // Both indexed Gets precede subtraction's left-to-right ToNumber calls.
        const auto a = times.get(i), b = times.get(j);
        return Interpolant_detail::subtract(a, b);
    });
    for (double i = 0; i != n; ++i) result.set(i, order[static_cast<std::size_t>(i)]);
    return result;
}
AnimationArray sortedArray(AnimationArray values, double stride, AnimationArray order) {
    const auto n = Animation_detail::length(values);
    auto result = values.allocate(n);
    if (n != 0) {
        Animation_detail::requireStride(stride);
        if (stride == 0 || std::fmod(n, stride) != 0) throw JSRangeError("sortedArray would cause a nonterminating loop");
    }
    for (double i = 0, dst = 0; dst != n; ++i) {
        const auto source = order.number(i) * stride;
        for (double j = 0; j != stride; ++j) result.set(dst++, values.get(source + j));
    }
    return result;
}
void flattenJSON(JSValue keys, JSValue times, JSValue values, const std::string& property) {
    double i = 1;
    auto key = keys.get("0");
    while (!key.isUndefined() && key.get(property).isUndefined()) key = keys.get(JSValue(i++).toString());
    if (key.isUndefined()) return;
    auto value = key.get(property);
    if (value.isUndefined()) return;
    const bool array = value.type() == JSValue::Type::Array;
    const bool toArray = !array && !value.get("toArray").isUndefined();
    do {
        value = key.get(property);
        if (!value.isUndefined()) {
            push(times, key.get("time"));
            if (array) {
                if (value.type() != JSValue::Type::Array && value.type() != JSValue::Type::String)
                    throw JSTypeError("value is not iterable");
                std::vector<JSValue> elements;
                for (double j = 0; j < value.get("length").toNumber(); ++j) {
                    auto element = value.get(JSValue(j).toString());
                    if (value.type() == JSValue::Type::String) {
                        const auto unit = [](const JSValue& character) -> unsigned {
                            const auto& bytes = character.string();
                            if (bytes.size() != 3) return 0;
                            return (unsigned(static_cast<unsigned char>(bytes[0]) & 15) << 12) |
                                   (unsigned(static_cast<unsigned char>(bytes[1]) & 63) << 6) |
                                   unsigned(static_cast<unsigned char>(bytes[2]) & 63);
                        };
                        const auto first = unit(element);
                        if (first >= 0xd800 && first <= 0xdbff && j + 1 < value.get("length").toNumber()) {
                            const auto next = value.get(JSValue(j + 1).toString());
                            const auto second = unit(next);
                            if (second >= 0xdc00 && second <= 0xdfff) { element = element.string() + next.string(); ++j; }
                        }
                    }
                    elements.push_back(element);
                }
                for (const auto& element : elements) push(values, element);
            } else if (toArray) {
                const auto callback = value.get("toArray");
                callback.call(value, {values, values.length()});
            } else push(values, value);
        }
        key = keys.get(JSValue(i++).toString());
    } while (!key.isUndefined());
}
std::shared_ptr<AnimationClip> subclip(const AnimationClip& source, JSValue name,
                                      double start, double end, double fps) {
    auto clip = source.clone();
    clip->name = std::move(name);
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    for (std::size_t i = 0; i < clip->tracks->size(); ++i) {
        const auto track = trackAt(clip->tracks, i);
        const auto size = track->getValueSize();
        auto times = JSValue::array(), values = JSValue::array();
        for (double j = 0; j < Animation_detail::length(track->times); ++j) {
            const auto frame = track->times.number(j) * fps;
            if (frame < start || frame >= end) continue;
            push(times, track->times.get(j));
            for (double k = 0; k < size; ++k) push(values, track->values.get(j * size + k));
        }
        if (times.length() == 0) continue;
        track->times = convertArray(times, static_cast<AnimationArrayType>(track->times.storage().index()));
        track->values = convertArray(values, static_cast<AnimationArrayType>(track->values.storage().index()));
        tracks->push_back(track);
    }
    clip->tracks = tracks;
    JSValue minimum = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < clip->tracks->size(); ++i) {
        const auto a = minimum.toNumber(), b = trackAt(clip->tracks, i)->times.number(0);
        if (a > b) minimum = trackAt(clip->tracks, i)->times.get(0);
    }
    for (std::size_t i = 0; i < clip->tracks->size(); ++i) trackAt(clip->tracks, i)->shift(-1 * minimum.toNumber());
    clip->resetDuration();
    return clip;
}
AnimationClip& makeClipAdditive(AnimationClip& target, double frame, const AnimationClip* reference, double fps) {
    if (!reference) reference = &target;
    if (fps <= 0) fps = 30;
    const auto count = reference->tracks->size();
    const auto time = frame / fps;
    for (std::size_t i = 0; i < count; ++i) {
        const auto ref = trackAt(reference->tracks, i);
        const auto type = ref->ValueTypeName;
        if (type == "bool" || type == "string") continue;
        std::shared_ptr<KeyframeTrack> dst;
        const auto candidates = target.tracks;
        for (std::size_t j = 0; j < candidates->size(); ++j) {
            auto track = trackAt(candidates, j);
            if (track->name.strictEquals(ref->name) && track->ValueTypeName == type) { dst = track; break; }
        }
        if (!dst) continue;
        const auto refSize = ref->getValueSize();
        const auto refFactory = ref->createInterpolant.function();
        const auto refOffset = refFactory && refFactory->isInterpolantFactoryMethodGLTFCubicSpline ? refSize / 3 : 0;
        const auto dstSize = dst->getValueSize();
        const auto dstFactory = dst->createInterpolant.function();
        const auto dstOffset = dstFactory && dstFactory->isInterpolantFactoryMethodGLTFCubicSpline ? dstSize / 3 : 0;
        const auto last = Animation_detail::length(ref->times) - 1;
        AnimationArray value;
        if (time <= ref->times.number(0)) value = Animation_detail::slice(ref->values, refOffset, refSize - refOffset);
        else if (time >= ref->times.number(last)) {
            const auto start = last * refSize + refOffset;
            value = Animation_detail::slice(ref->values, start, start + refSize - refOffset);
        } else {
            const auto interpolant = ref->createInterpolant();
            interpolant->evaluate(time);
            value = Animation_detail::slice(interpolant->resultBuffer);
        }
        if (type == "quaternion") {
            std::array<double, 4> q{value.number(0), value.number(1), value.number(2), value.number(3)};
            auto length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
            if (length == 0) q = {0, 0, 0, 1};
            else { length = 1 / length; for (auto& component : q) component *= length; }
            q[0] = -q[0]; q[1] = -q[1]; q[2] = -q[2];
            for (std::size_t j = 0; j < 4; ++j) value.set(double(j), q[j]);
        }
        const auto n = Animation_detail::length(dst->times);
        for (double j = 0; j < n; ++j) {
            const auto start = j * dstSize + dstOffset;
            if (type == "quaternion") {
                std::array<JSValue, 4> a{value.get(0), value.get(1), value.get(2), value.get(3)};
                auto destination = dst->values;
                std::array<JSValue, 4> b{destination.get(start), destination.get(start + 1), destination.get(start + 2), destination.get(start + 3)};
                const auto mul = [&](int x, int y) { return Interpolant_detail::multiply(a[x], b[y]); };
                auto x = mul(0, 3); x += mul(3, 0); x += mul(1, 2); x -= mul(2, 1); destination.set(start, x);
                auto y = mul(1, 3); y += mul(3, 1); y += mul(2, 0); y -= mul(0, 2); destination.set(start + 1, y);
                auto z = mul(2, 3); z += mul(3, 2); z += mul(0, 1); z -= mul(1, 0); destination.set(start + 2, z);
                auto w = mul(3, 3); w -= mul(0, 0); w -= mul(1, 1); w -= mul(2, 2); destination.set(start + 3, w);
            } else {
                const auto end = dstSize - dstOffset * 2;
                for (double k = 0; k < end; ++k) {
                    auto destination = dst->values;
                    const auto a = destination.get(start + k), b = value.get(k);
                    destination.set(start + k, Interpolant_detail::subtract(a, b));
                }
            }
        }
    }
    target.blendMode = AdditiveAnimationBlendMode;
    return target;
}

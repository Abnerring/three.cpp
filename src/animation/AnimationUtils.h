#pragma once

#include "math/Interpolant.h"
#include <memory>
#include <optional>
#include <string>

class AnimationClip;
using AnimationArray = InterpolantArray;
enum class AnimationArrayType { Int8, Uint8, Int16, Uint16, Int32, Uint32, Float32, Float64,
                                Uint8Clamped, Array };

// A shared settings object. JSValue objects retain their accessors; the pair
// constructor also accepts TypedArray handles without copying their storage.
class AnimationSettings {
  public:
    struct Tangents { AnimationArray in, out; };
    using Storage = std::variant<JSValue, std::shared_ptr<Tangents>>;
    const Storage& storage() const { return value_; }
  private:
    Storage value_;
  public:
    AnimationSettings() : value_(JSValue{}) {}
    AnimationSettings(JSValue value) : value_(std::move(value)) {}
    explicit AnimationSettings(std::shared_ptr<Tangents> value) : value_(std::move(value)) {}
    AnimationSettings(AnimationArray in, AnimationArray out)
        : value_(std::make_shared<Tangents>(Tangents{std::move(in), std::move(out)})) {}
    bool isUndefined() const;
    bool toBoolean() const;
    bool sameIdentity(const AnimationSettings& other) const;
    AnimationArray get(const std::string& key) const;
    void set(const std::string& key, AnimationArray value);
};

AnimationArray convertArray(AnimationArray array, AnimationArrayType type);
bool isTypedArray(const AnimationArray& array);
bool hasTangents(AnimationSettings settings);
AnimationArray getKeyframeOrder(AnimationArray times);
AnimationArray sortedArray(AnimationArray values, double stride, AnimationArray order);
void flattenJSON(JSValue jsonKeys, JSValue times, JSValue values, const std::string& valuePropertyName);
std::shared_ptr<AnimationClip> subclip(const AnimationClip& sourceClip, JSValue name,
                                      double startFrame, double endFrame, double fps = 30);
AnimationClip& makeClipAdditive(AnimationClip& targetClip, double referenceFrame = 0,
                                const AnimationClip* referenceClip = nullptr, double fps = 30);

class AnimationUtils {
  public:
    static AnimationArray convertArray(AnimationArray a, AnimationArrayType t) { return ::convertArray(std::move(a), t); }
    static bool isTypedArray(const AnimationArray& a) { return ::isTypedArray(a); }
    static bool hasTangents(const AnimationSettings& s) { return ::hasTangents(s); }
    static AnimationArray getKeyframeOrder(const AnimationArray& a) { return ::getKeyframeOrder(a); }
    static AnimationArray sortedArray(const AnimationArray& v, double s, const AnimationArray& o) { return ::sortedArray(v, s, o); }
    static void flattenJSON(JSValue k, JSValue t, JSValue v, const std::string& p) { ::flattenJSON(k, t, v, p); }
    static std::shared_ptr<AnimationClip> subclip(const AnimationClip& c, JSValue n, double s, double e, double fps = 30) { return ::subclip(c, std::move(n), s, e, fps); }
    static AnimationClip& makeClipAdditive(AnimationClip& t, double f = 0, const AnimationClip* r = nullptr, double fps = 30) { return ::makeClipAdditive(t, f, r, fps); }
};

namespace Animation_detail {
class JSONSyntaxError : public std::runtime_error { public: using std::runtime_error::runtime_error; };
AnimationArray slice(AnimationArray array, double begin = 0, std::optional<double> end = {});
JSValue ordinary(const AnimationArray& array);
JSValue lengthValue(const AnimationArray& array);
double length(const AnimationArray& array);
void requireStride(double stride);
JSValue parseJSON(const std::string& text);
std::optional<std::string> stringifyJSON(const JSValue& value);
}

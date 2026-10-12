#pragma once

#include "animation/AnimationUtils.h"
#include "constants.h"
#include <functional>

class KeyframeTrack;
class InterpolantFactory {
  public:
    using Callback = std::function<std::shared_ptr<Interpolant>(KeyframeTrack&, AnimationArray)>;
    struct Function {
        Callback callback;
        bool isInterpolantFactoryMethodGLTFCubicSpline = false;
        JSValue runtimeFunction;
    };
    using FunctionPtr = std::shared_ptr<Function>;
    explicit InterpolantFactory(KeyframeTrack& owner) : owner_(&owner) {}
    InterpolantFactory(const InterpolantFactory&) = delete;
    InterpolantFactory& operator=(const InterpolantFactory& other) { function_ = other.function_; return *this; }
    InterpolantFactory& operator=(FunctionPtr function) { function_ = std::move(function); return *this; }
    std::shared_ptr<Interpolant> operator()(AnimationArray result = {}) const;
    FunctionPtr function() const { return function_; }
    static FunctionPtr custom(Callback callback, bool gltfCubicSpline = false) {
        return std::make_shared<Function>(Function{std::move(callback), gltfCubicSpline});
    }
  private:
    KeyframeTrack* owner_;
    FunctionPtr function_;
};

class KeyframeTrack : public std::enable_shared_from_this<KeyframeTrack> {
  public:
    using Array = AnimationArray;
    JSValue name;
    Array times, values;
    AnimationSettings settings;
    std::string ValueTypeName;
    AnimationArrayType TimeBufferType = AnimationArrayType::Float32;
    AnimationArrayType ValueBufferType = AnimationArrayType::Float32;
    JSValue DefaultInterpolation = InterpolateLinear;
    InterpolantFactory createInterpolant{*this};

    KeyframeTrack(JSValue name = {}, Array times = {}, Array values = {}, JSValue interpolation = {});
    virtual ~KeyframeTrack() = default;
    KeyframeTrack(const KeyframeTrack&) = delete;
    KeyframeTrack& operator=(const KeyframeTrack&) = delete;
    static JSValue toJSON(const KeyframeTrack& track);
    virtual std::shared_ptr<Interpolant> InterpolantFactoryMethodDiscrete(Array result = {});
    virtual std::shared_ptr<Interpolant> InterpolantFactoryMethodLinear(Array result = {});
    virtual std::shared_ptr<Interpolant> InterpolantFactoryMethodSmooth(Array result = {});
    virtual std::shared_ptr<Interpolant> InterpolantFactoryMethodBezier(Array result = {});
    virtual KeyframeTrack& setInterpolation(JSValue interpolation);
    virtual JSValue getInterpolation() const;
    virtual double getValueSize() const;
    virtual KeyframeTrack& shift(JSValue timeOffset);
    virtual KeyframeTrack& scale(JSValue timeScale);
    virtual KeyframeTrack& trim(double startTime, double endTime);
    virtual bool validate() const;
    virtual KeyframeTrack& optimize();
    virtual std::shared_ptr<KeyframeTrack> clone() const;
  protected:
    struct Profile { std::string type; AnimationArrayType values; int interpolation; bool linear, smooth; };
    KeyframeTrack(JSValue name, Array times, Array values, JSValue interpolation, Profile profile);
    virtual std::shared_ptr<KeyframeTrack> constructClone(JSValue name, Array times, Array values) const;
    virtual bool supportsInterpolation(int mode) const;
  private:
    bool linear_ = true, smooth_ = true, quaternionLinear_ = false;
    InterpolantFactory::FunctionPtr builtinFactory(int mode) const;
};

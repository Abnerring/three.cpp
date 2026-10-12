#pragma once
#include "animation/AnimationUtils.h"
#include <functional>
#include <type_traits>

// Copies retain function identity, including mutable captures. An invocation
// also retains that identity when the callback replaces its owning property.
template <class... Args> class PropertyMixerCallback {
  public:
    using Function = std::function<void(Args...)>;
    struct Target { Function callback; JSValue runtimeFunction; std::string runtimeName; };
    PropertyMixerCallback() = default;
    PropertyMixerCallback(std::nullptr_t) {}
    template <class F> requires (!std::is_same_v<std::remove_cvref_t<F>, PropertyMixerCallback> &&
                                std::is_invocable_r_v<void, F&, Args...>)
    PropertyMixerCallback(F&& callback) { *this = Function(std::forward<F>(callback)); }
    PropertyMixerCallback& operator=(Function callback) {
        target = callback ? std::make_shared<Target>(Target{std::move(callback), {}}) : nullptr;
        return *this;
    }
    template <class F> requires (!std::is_same_v<std::remove_cvref_t<F>, PropertyMixerCallback> &&
                                std::is_invocable_r_v<void, F&, Args...>)
    PropertyMixerCallback& operator=(F&& callback) {
        return *this = Function(std::forward<F>(callback));
    }
    PropertyMixerCallback& operator=(std::nullptr_t) { target.reset(); return *this; }
    explicit operator bool() const { return bool(target); }
    void operator()(Args... args) const {
        const auto retained = target;
        if (!retained) throw JSTypeError("PropertyMixer callback is not callable");
        retained->callback(std::forward<Args>(args)...);
    }
    bool sameIdentity(const PropertyMixerCallback& other) const { return target == other.target; }
    const std::shared_ptr<Target>& runtimeTarget() const { return target; }
    const PropertyMixerCallback& named(const char* name) const {
        if (target) target->runtimeName = name;
        return *this;
    }
    static PropertyMixerCallback managed(JSValue function,
        std::function<void(const JSValue&, Args...)> dispatch) {
        PropertyMixerCallback value;
        value.target = std::make_shared<Target>();
        value.target->runtimeFunction = std::move(function);
        const std::weak_ptr<Target> weak = value.target;
        value.target->callback = [weak, dispatch = std::move(dispatch)](Args... args) {
            const auto retained = weak.lock();
            dispatch(retained->runtimeFunction, std::forward<Args>(args)...);
        };
        return value;
    }
  private:
    std::shared_ptr<Target> target;
};

// PropertyBinding and composite bindings implement the same operations. Array
// handles and offsets are passed unchanged; callbacks may replace the mixer's
// binding or buffer without invalidating the receiver or arguments in flight.
class PropertyBindingInterface : public std::enable_shared_from_this<PropertyBindingInterface> {
  public:
    virtual ~PropertyBindingInterface() = default;
    virtual void getValue(AnimationArray buffer, double offset) = 0;
    virtual void setValue(AnimationArray buffer, double offset) = 0;
};

class PropertyMixer : public std::enable_shared_from_this<PropertyMixer> {
  public:
    using Array = AnimationArray;
    using MixFunction = PropertyMixerCallback<PropertyMixer&, Array, double, double, JSValue, double>;
    using IdentityFunction = PropertyMixerCallback<PropertyMixer&>;
    std::shared_ptr<PropertyBindingInterface> binding;
    double valueSize;
    Array buffer;
    JSValue cumulativeWeight = 0, cumulativeWeightAdditive = 0;
    double useCount = 0, referenceCount = 0;
    double _origIndex = 3, _addIndex = 4;
    JSValue _workIndex;
    JSValue _cacheIndex;
    MixFunction _mixBufferRegion, _mixBufferRegionAdditive;
    IdentityFunction _setIdentity;

    PropertyMixer(std::shared_ptr<PropertyBindingInterface> binding, JSValue typeName, double valueSize);
    virtual ~PropertyMixer() = default;
    virtual void accumulate(JSValue accuIndex, JSValue weight);
    virtual void accumulateAdditive(JSValue weight);
    virtual void apply(JSValue accuIndex);
    virtual void saveOriginalState();
    virtual void restoreOriginalState();
    virtual void _setAdditiveIdentityNumeric();
    virtual void _setAdditiveIdentityQuaternion();
    virtual void _setAdditiveIdentityOther();
    virtual void _select(Array buffer, double dstOffset, double srcOffset, JSValue t, double stride);
    virtual void _slerp(Array buffer, double dstOffset, double srcOffset, JSValue t);
    virtual void _slerpAdditive(Array buffer, double dstOffset, double srcOffset, JSValue t, double stride);
    virtual void _lerp(Array buffer, double dstOffset, double srcOffset, JSValue t, double stride);
    virtual void _lerpAdditive(Array buffer, double dstOffset, double srcOffset, JSValue t, double stride);
};

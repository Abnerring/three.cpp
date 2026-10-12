#pragma once
#include "core/JSValue.h"
#include "core/JSMember.h"

// A value handle to the managed action object. All state and cross-object links
// live in the JSValue graph, including mixer/action cycles.
class AnimationAction {
  public:
    AnimationAction(JSValue mixer, JSValue clip, JSValue localRoot = nullptr, JSValue blendMode = {});
    AnimationAction(const AnimationAction& other) : AnimationAction(other.object_, Existing{}) {}
    AnimationAction(AnimationAction&& other) noexcept : AnimationAction(std::move(other.object_), Existing{}) {}
    AnimationAction& operator=(const AnimationAction& other) { object_ = other.object_; return *this; }
    AnimationAction& operator=(AnimationAction&& other) noexcept { object_ = std::move(other.object_); return *this; }
    static AnimationAction fromObject(JSValue object);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    explicit operator bool() const { return object_.toBoolean(); }
    bool operator==(const AnimationAction& other) const { return object_.sameValue(other.object_); }
    JSValue get(const std::string& key) const { return object_.get(key); }
    void set(const std::string& key, JSValue value) { object_.set(key, value); }
    static JSValue prototype();
    JSMember time{&object_, "time"}, timeScale{&object_, "timeScale"}, weight{&object_, "weight"};
    JSMember loop{&object_, "loop"}, repetitions{&object_, "repetitions"}, blendMode{&object_, "blendMode"};
    JSMember paused{&object_, "paused"}, enabled{&object_, "enabled"}, clampWhenFinished{&object_, "clampWhenFinished"};
    JSMember zeroSlopeAtStart{&object_, "zeroSlopeAtStart"}, zeroSlopeAtEnd{&object_, "zeroSlopeAtEnd"};

    JSValue play();
    JSValue stop();
    JSValue reset();
    JSValue isRunning();
    JSValue isScheduled();
    JSValue startAt(JSValue time);
    JSValue setLoop(JSValue mode, JSValue repetitions);
    JSValue setEffectiveWeight(JSValue weight);
    JSValue getEffectiveWeight();
    JSValue fadeIn(JSValue duration);
    JSValue fadeOut(JSValue duration);
    JSValue crossFadeFrom(JSValue action, JSValue duration, JSValue warp = false);
    JSValue crossFadeTo(JSValue action, JSValue duration, JSValue warp = false);
    JSValue stopFading();
    JSValue setEffectiveTimeScale(JSValue timeScale);
    JSValue getEffectiveTimeScale();
    JSValue setDuration(JSValue duration);
    JSValue syncWith(JSValue action);
    JSValue halt(JSValue duration);
    JSValue warp(JSValue startTimeScale, JSValue endTimeScale, JSValue duration);
    JSValue stopWarping();
    JSValue getMixer();
    JSValue getClip();
    JSValue getRoot();
    JSValue _update(JSValue time, JSValue deltaTime, JSValue timeDirection, JSValue accuIndex);
    JSValue _updateWeight(JSValue time);
    JSValue _updateTimeScale(JSValue time);
    JSValue _updateTime(JSValue deltaTime);
    JSValue _setEndings(JSValue atStart, JSValue atEnd, JSValue pingPong);
    JSValue _scheduleFading(JSValue duration, JSValue weightNow, JSValue weightThen);
  private:
    struct Existing {};
    AnimationAction(JSValue value, Existing) : object_(std::move(value)) {}
    JSValue object_;
};

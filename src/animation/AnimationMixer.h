#pragma once
#include "animation/AnimationAction.h"
#include "core/JSMember.h"
#include <memory>

class AnimationClip;
class Object3D;
class AnimationMixer {
    JSValue object_;
    struct Existing {};
    AnimationMixer(JSValue object, Existing) : object_(std::move(object)) {}
  public:
    JSMember time{&object_, "time"}, timeScale{&object_, "timeScale"}, stats{&object_, "stats"};
    JSMember _root{&object_, "_root"}, _accuIndex{&object_, "_accuIndex"};
    JSMember _actions{&object_, "_actions"}, _nActiveActions{&object_, "_nActiveActions"},
        _actionsByClip{&object_, "_actionsByClip"};
    JSMember _bindings{&object_, "_bindings"}, _nActiveBindings{&object_, "_nActiveBindings"},
        _bindingsByRootAndName{&object_, "_bindingsByRootAndName"};
    JSMember _controlInterpolants{&object_, "_controlInterpolants"},
        _nActiveControlInterpolants{&object_, "_nActiveControlInterpolants"};
    explicit AnimationMixer(JSValue root);
    explicit AnimationMixer(const std::shared_ptr<Object3D>& root);
    AnimationMixer(const AnimationMixer& other) : AnimationMixer(other.object_, Existing{}) {}
    AnimationMixer(AnimationMixer&& other) noexcept : AnimationMixer(std::move(other.object_), Existing{}) {}
    AnimationMixer& operator=(const AnimationMixer& other) { object_ = other.object_; return *this; }
    AnimationMixer& operator=(AnimationMixer&& other) noexcept { object_ = std::move(other.object_); return *this; }
    static AnimationMixer fromObject(JSValue object) { return AnimationMixer(std::move(object), Existing{}); }
    static JSValue prototype();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue get(const std::string& key) const { return object_.get(key); }
    void set(const std::string& key, JSValue value) { object_.set(key,value); }
    AnimationAction clipAction(JSValue clip, JSValue optionalRoot = {}, JSValue blendMode = {});
    AnimationAction clipAction(const std::shared_ptr<AnimationClip>& clip, JSValue optionalRoot = {}, JSValue blendMode = {});
    AnimationAction existingAction(JSValue clip, JSValue optionalRoot = {});
    AnimationAction existingAction(const std::shared_ptr<AnimationClip>& clip, JSValue optionalRoot = {});
    JSValue stopAllAction();
    JSValue update(JSValue deltaTime);
    JSValue setTime(JSValue time);
    JSValue getRoot();
    JSValue uncacheClip(JSValue clip);
    JSValue uncacheClip(const std::shared_ptr<AnimationClip>& clip);
    JSValue uncacheRoot(JSValue root);
    JSValue uncacheAction(JSValue clip, JSValue optionalRoot = {});
    JSValue addEventListener(JSValue type, JSValue listener);
    JSValue hasEventListener(JSValue type, JSValue listener);
    JSValue removeEventListener(JSValue type, JSValue listener);
    JSValue dispatchEvent(JSValue event);
    JSValue _bindAction(JSValue action, JSValue prototypeAction = {});
    JSValue _activateAction(JSValue action);
    JSValue _deactivateAction(JSValue action);
    JSValue _initMemoryManager();
    JSValue _isActiveAction(JSValue action);
    JSValue _addInactiveAction(JSValue action, JSValue clipUuid, JSValue rootUuid);
    JSValue _removeInactiveAction(JSValue action);
    JSValue _removeInactiveBindingsForAction(JSValue action);
    JSValue _lendAction(JSValue action);
    JSValue _takeBackAction(JSValue action);
    JSValue _addInactiveBinding(JSValue binding, JSValue rootUuid, JSValue trackName);
    JSValue _removeInactiveBinding(JSValue binding);
    JSValue _lendBinding(JSValue binding);
    JSValue _takeBackBinding(JSValue binding);
    JSValue _lendControlInterpolant();
    JSValue _takeBackControlInterpolant(JSValue interpolant);
};

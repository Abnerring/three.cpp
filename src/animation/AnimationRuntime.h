#pragma once
#include "core/JSValue.h"
#include <memory>
#include <vector>

class PropertyBindingInterface;
class PropertyMixer;
class Interpolant;
class AnimationClip;
class KeyframeTrack;

JSValue nativePropertyBinding(const std::shared_ptr<PropertyBindingInterface>& binding);
JSValue nativePropertyMixer(const std::shared_ptr<PropertyMixer>& mixer);
JSValue nativeInterpolant(const std::shared_ptr<Interpolant>& interpolant);
JSValue nativeAnimationClip(const std::shared_ptr<AnimationClip>& clip);
JSValue nativeKeyframeTrack(const std::shared_ptr<KeyframeTrack>& track);
JSValue nativeAnimationClips(const std::shared_ptr<std::vector<std::shared_ptr<AnimationClip>>>& clips);
std::vector<JSNativeOwnerNode> nativeAnimationClipOwners(
    const std::shared_ptr<void>& owner, const std::vector<std::shared_ptr<AnimationClip>>& clips);

std::shared_ptr<PropertyBindingInterface> propertyBindingFromNative(const JSValue& value);
std::shared_ptr<PropertyMixer> propertyMixerFromNative(const JSValue& value);
std::shared_ptr<Interpolant> interpolantFromNative(const JSValue& value);
std::shared_ptr<AnimationClip> animationClipFromNative(const JSValue& value);
std::shared_ptr<KeyframeTrack> keyframeTrackFromNative(const JSValue& value);

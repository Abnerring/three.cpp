#pragma once
#include "animation/KeyframeTrack.h"

class Object3D;
class AnimationClip {
  public:
    using Tracks = std::vector<std::shared_ptr<KeyframeTrack>>;
    using TracksPtr = std::shared_ptr<Tracks>;
    using Clips = std::vector<std::shared_ptr<AnimationClip>>;
    JSValue name, uuid, blendMode;
    double duration;
    TracksPtr tracks;
    JSValue userData = JSValue::object();
    AnimationClip(JSValue name = "", double duration = -1,
                    TracksPtr tracks = std::make_shared<Tracks>(), JSValue blendMode = NormalAnimationBlendMode);
    virtual ~AnimationClip() = default;
    static std::shared_ptr<AnimationClip> parse(JSValue json);
    static std::shared_ptr<AnimationClip> parse(const JsonValue& json) { return parse(JSValue::fromJSON(json)); }
    static JSValue toJSON(const AnimationClip& clip);
    virtual JSValue toJSON() const { return toJSON(*this); }
    JsonValue toJSONValue() const { return toJSON().toJSON().value(); }
    static std::shared_ptr<AnimationClip> CreateFromMorphTargetSequence(JSValue name, JSValue sequence,
                                                                       double fps, bool noLoop = false);
    static Clips CreateClipsFromMorphTargetSequences(JSValue morphTargets, double fps, bool noLoop = false);
    static std::shared_ptr<AnimationClip> findByName(const Clips& clips, const JSValue& name);
    static std::shared_ptr<AnimationClip> findByName(const Object3D& object, const JSValue& name);
    static JSValue findByName(JSValue objectOrClipArray, JSValue name);
    virtual AnimationClip& resetDuration();
    virtual AnimationClip& trim();
    virtual bool validate() const;
    virtual AnimationClip& optimize();
    virtual std::shared_ptr<AnimationClip> clone() const;
  protected:
    virtual std::shared_ptr<AnimationClip> constructClone(JSValue name, double duration, TracksPtr tracks,
                                                          JSValue blendMode) const;
};

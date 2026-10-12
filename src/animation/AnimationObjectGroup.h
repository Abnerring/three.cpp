#pragma once
#include "core/JSMember.h"
#include <initializer_list>
#include <vector>

// The JavaScript object is the authoritative state. Copying this C++ handle
// preserves object identity, including arrays, descriptors and callback edges.
class AnimationObjectGroup {
    JSValue object_;
  public:
    JSMember isAnimationObjectGroup{&object_, "isAnimationObjectGroup"}, uuid{&object_, "uuid"},
        nCachedObjects_{&object_, "nCachedObjects_"}, stats{&object_, "stats"},
        _objects{&object_, "_objects"}, _indicesByUUID{&object_, "_indicesByUUID"},
        _paths{&object_, "_paths"}, _parsedPaths{&object_, "_parsedPaths"},
        _bindings{&object_, "_bindings"}, _bindingsIndicesByPath{&object_, "_bindingsIndicesByPath"};
    explicit AnimationObjectGroup(const std::vector<JSValue>& objects = {});
    AnimationObjectGroup(std::initializer_list<JSValue> objects)
        : AnimationObjectGroup(std::vector<JSValue>(objects)) {}
    AnimationObjectGroup(const AnimationObjectGroup& other) : object_(other.object_) {}
    AnimationObjectGroup(AnimationObjectGroup&& other) noexcept : object_(std::move(other.object_)) {}
    AnimationObjectGroup& operator=(const AnimationObjectGroup& other) { object_ = other.object_; return *this; }
    AnimationObjectGroup& operator=(AnimationObjectGroup&& other) noexcept { object_ = std::move(other.object_); return *this; }
    static AnimationObjectGroup fromObject(JSValue object);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    bool operator==(const AnimationObjectGroup& other) const { return object_.sameValue(other.object_); }
    JSValue add(const std::vector<JSValue>& objects = {});
    JSValue remove(const std::vector<JSValue>& objects = {});
    JSValue uncache(const std::vector<JSValue>& objects = {});
    JSValue subscribe_(JSValue path, JSValue parsedPath = {});
    JSValue unsubscribe_(JSValue path);
    static JSValue prototype();
  private:
    struct Existing {};
    AnimationObjectGroup(JSValue object, Existing) : object_(std::move(object)) {}
};

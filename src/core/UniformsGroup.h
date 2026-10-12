#pragma once
#include "core/JSMember.h"

// Uniforms, nested arrays, callbacks and event listeners are managed graph edges.
// ShaderMaterial.uniformsGroups accepts object() or the implicit JSValue handle.
class UniformsGroup {
    JSValue object_;
    struct Existing {};
    UniformsGroup(JSValue object, Existing) : object_(std::move(object)) {}
  public:
    JSMember isUniformsGroup{&object_, "isUniformsGroup"}, id{&object_, "id"},
        name{&object_, "name"}, usage{&object_, "usage"}, uniforms{&object_, "uniforms"};
    UniformsGroup();
    UniformsGroup(const UniformsGroup& other) : UniformsGroup(other.object_, Existing{}) {}
    UniformsGroup(UniformsGroup&& other) noexcept : UniformsGroup(std::move(other.object_), Existing{}) {}
    UniformsGroup& operator=(const UniformsGroup& other) { object_ = other.object_; return *this; }
    UniformsGroup& operator=(UniformsGroup&& other) noexcept { object_ = std::move(other.object_); return *this; }
    static UniformsGroup fromObject(JSValue object) { return UniformsGroup(std::move(object), Existing{}); }
    static JSValue prototype();
    static JSValue constructor();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue add(JSValue uniform = {});
    JSValue remove(JSValue uniform = {});
    JSValue setName(JSValue name = {});
    JSValue setUsage(JSValue usage = {});
    JSValue dispose();
    JSValue copy(JSValue source = {});
    JSValue clone() const;
    JSValue addEventListener(JSValue type, JSValue listener);
    JSValue hasEventListener(JSValue type, JSValue listener);
    JSValue removeEventListener(JSValue type, JSValue listener);
    JSValue dispatchEvent(JSValue event);
};

#pragma once
#include "core/JSMember.h"

// A handle into the managed object graph. C++ handle copies retain identity;
// clone() implements the distinct upstream Uniform.clone operation.
class Uniform {
    JSValue object_;
    struct Existing {};
    Uniform(JSValue object, Existing) : object_(std::move(object)) {}
  public:
    JSMember value{&object_, "value"};
    explicit Uniform(JSValue value = {});
    Uniform(const Uniform& other) : Uniform(other.object_, Existing{}) {}
    Uniform(Uniform&& other) noexcept : Uniform(std::move(other.object_), Existing{}) {}
    Uniform& operator=(const Uniform& other) { object_ = other.object_; return *this; }
    Uniform& operator=(Uniform&& other) noexcept { object_ = std::move(other.object_); return *this; }
    static Uniform fromObject(JSValue object) { return Uniform(std::move(object), Existing{}); }
    static JSValue prototype();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    Uniform clone() const;
};

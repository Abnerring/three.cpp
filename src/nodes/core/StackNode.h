#pragma once
#include "core/JSMember.h"

JSValue nativeStackNodeConstructor();
JSValue nativeGetCurrentStack();
void nativeSetCurrentStack(const JSValue& stack);
class StackNode {
    JSValue object_;
  public:
    explicit StackNode(const JSValue& parent = nullptr);
    StackNode(const StackNode& other) : object_(other.object_) {}
    StackNode& operator=(const StackNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeStackNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember nodes{&object_, "nodes"}, outputNode{&object_, "outputNode"}, parent{&object_, "parent"};
    JSValue addToStack(const JSValue& node, const JSValue& index = -1) const;
    JSValue addToStackBefore(const JSValue& node) const;
    JSValue build(const JSValue& builder, const std::vector<JSValue>& params = {}) const;
};

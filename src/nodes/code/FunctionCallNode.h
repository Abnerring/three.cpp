#pragma once
#include "core/JSMember.h"

JSValue nativeFunctionCallNodeConstructor();
JSValue nativeFunctionCallNodeModule();
class FunctionCallNode {
    JSValue object_;
  public:
    explicit FunctionCallNode(const JSValue& functionNode = nullptr, const JSValue& parameters = {});
    FunctionCallNode(const FunctionCallNode& other) : object_(other.object_) {}
    FunctionCallNode& operator=(const FunctionCallNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeFunctionCallNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember functionNode{&object_, "functionNode"}, parameters{&object_, "parameters"};
    JSValue setParameters(const JSValue& value) const;
    JSValue getParameters() const;
    JSValue generate(const JSValue& builder) const;
};

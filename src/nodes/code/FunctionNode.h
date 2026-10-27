#pragma once
#include "core/JSMember.h"

JSValue nativeFunctionNodeConstructor();
JSValue nativeFunctionNodeModule();
class FunctionNode {
    JSValue object_;
  public:
    explicit FunctionNode(const JSValue& code = "", const JSValue& includes = {}, const JSValue& language = "");
    FunctionNode(const FunctionNode& other) : object_(other.object_) {}
    FunctionNode& operator=(const FunctionNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeFunctionNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember code{&object_, "code"}, includes{&object_, "includes"}, language{&object_, "language"};
    JSValue getNodeFunction(const JSValue& builder) const;
    JSValue getInputs(const JSValue& builder) const;
    JSValue generate(const JSValue& builder, const JSValue& output = {}) const;
};

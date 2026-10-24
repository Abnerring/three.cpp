#pragma once
#include "core/JSMember.h"

JSValue nativeNodeFunctionConstructor();
class NodeFunction {
    JSValue object_;
  public:
    NodeFunction(const JSValue& type = {}, const JSValue& inputs = {}, const JSValue& name = "", const JSValue& precision = "");
    NodeFunction(const NodeFunction& other) : object_(other.object_) {}
    NodeFunction& operator=(const NodeFunction& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeNodeFunctionConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember type{&object_, "type"}, inputs{&object_, "inputs"}, name{&object_, "name"}, precision{&object_, "precision"};
    JSValue getCode(const JSValue& name = {}) const;
};

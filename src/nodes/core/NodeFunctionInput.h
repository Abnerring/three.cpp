#pragma once
#include "core/JSMember.h"

JSValue nativeNodeFunctionInputConstructor();
class NodeFunctionInput {
    JSValue object_;
  public:
    NodeFunctionInput(const JSValue& type = {}, const JSValue& name = {}, const JSValue& count = nullptr, const JSValue& qualifier = "", const JSValue& isConst = false);
    NodeFunctionInput(const NodeFunctionInput& other) : object_(other.object_) {}
    NodeFunctionInput& operator=(const NodeFunctionInput& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeNodeFunctionInputConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember type{&object_, "type"}, name{&object_, "name"}, count{&object_, "count"}, qualifier{&object_, "qualifier"}, isConst{&object_, "isConst"};
};

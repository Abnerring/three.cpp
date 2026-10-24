#pragma once
#include "core/JSMember.h"

JSValue nativeWGSLNodeFunctionConstructor();
class WGSLNodeFunction {
    JSValue object_;
  public:
    explicit WGSLNodeFunction(const JSValue& source = {});
    WGSLNodeFunction(const WGSLNodeFunction& other) : object_(other.object_) {}
    WGSLNodeFunction& operator=(const WGSLNodeFunction& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeWGSLNodeFunctionConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember type{&object_, "type"}, inputs{&object_, "inputs"}, name{&object_, "name"}, precision{&object_, "precision"};
    JSMember inputsCode{&object_, "inputsCode"}, blockCode{&object_, "blockCode"}, outputType{&object_, "outputType"};
    JSValue getCode(const JSValue& name = {}) const;
};

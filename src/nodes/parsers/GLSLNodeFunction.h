#pragma once
#include "core/JSMember.h"

JSValue nativeGLSLNodeFunctionConstructor();
class GLSLNodeFunction {
    JSValue object_;
  public:
    explicit GLSLNodeFunction(const JSValue& source = {});
    GLSLNodeFunction(const GLSLNodeFunction& other) : object_(other.object_) {}
    GLSLNodeFunction& operator=(const GLSLNodeFunction& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeGLSLNodeFunctionConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember type{&object_, "type"}, inputs{&object_, "inputs"}, name{&object_, "name"}, precision{&object_, "precision"};
    JSMember inputsCode{&object_, "inputsCode"}, blockCode{&object_, "blockCode"}, headerCode{&object_, "headerCode"};
    JSValue getCode(const JSValue& name = {}) const;
};

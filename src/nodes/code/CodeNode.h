#pragma once
#include "core/JSMember.h"

JSValue nativeCodeNodeConstructor();
class CodeNode {
    JSValue object_;
  public:
    explicit CodeNode(const JSValue& code = "", const JSValue& includes = {}, const JSValue& language = "");
    CodeNode(const CodeNode& other) : object_(other.object_) {}
    CodeNode& operator=(const CodeNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeCodeNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember code{&object_, "code"}, includes{&object_, "includes"}, language{&object_, "language"};
    JSValue setIncludes(const JSValue& includes) const;
    JSValue getIncludes(const JSValue& builder = {}) const;
    JSValue generate(const JSValue& builder) const;
};

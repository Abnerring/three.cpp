#pragma once

#include "core/JSValue.h"

JSValue nativeBasicEnvironmentNodeModule();
JSValue nativeBasicEnvironmentNodeBinding(const std::string& name);
JSValue nativeBasicEnvironmentNodeConstructor();

class BasicEnvironmentNode {
    JSValue object_;
  public:
    explicit BasicEnvironmentNode(const JSValue& envNode = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeBasicEnvironmentNodeConstructor(); }
};

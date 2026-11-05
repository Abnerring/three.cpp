#pragma once

#include "core/JSValue.h"

JSValue nativeEnvironmentNodeModule();
JSValue nativeEnvironmentNodeBinding(const std::string& name);
JSValue nativeEnvironmentNodeConstructor();

class EnvironmentNode {
    JSValue object_;
  public:
    explicit EnvironmentNode(const JSValue& envNode = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeEnvironmentNodeConstructor(); }
};

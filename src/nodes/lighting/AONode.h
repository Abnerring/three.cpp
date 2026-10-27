#pragma once
#include "core/JSValue.h"
JSValue nativeAONodeConstructor();
JSValue nativeAONodeModule();
JSValue nativeAONodeBinding(const std::string& name);
class AONode {
    JSValue object_;
  public:
    explicit AONode(const JSValue& aoNode = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeAONodeBinding("default"); }
};

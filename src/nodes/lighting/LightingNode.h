#pragma once
#include "core/JSValue.h"
JSValue nativeLightingNodeConstructor();
JSValue nativeLightingNodeModule();
JSValue nativeLightingNodeBinding(const std::string& name);
class LightingNode {
    JSValue object_;
  public:
    explicit LightingNode();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeLightingNodeBinding("default"); }
};

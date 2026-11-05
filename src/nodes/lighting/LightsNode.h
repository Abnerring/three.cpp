#pragma once

#include "core/JSValue.h"

JSValue nativeLightsNodeModule();
JSValue nativeLightsNodeBinding(const std::string& name);
JSValue nativeLightsNodeConstructor();
JSValue nativeLightsFactory();

class LightsNode {
    JSValue object_;
  public:
    explicit LightsNode();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeLightsNodeConstructor(); }
};

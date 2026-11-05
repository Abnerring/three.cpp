#pragma once

#include "core/JSValue.h"

JSValue nativeHemisphereLightNodeModule();
JSValue nativeHemisphereLightNodeBinding(const std::string& name);
JSValue nativeHemisphereLightNodeConstructor();

class HemisphereLightNode {
    JSValue object_;
  public:
    explicit HemisphereLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeHemisphereLightNodeConstructor(); }
};

#pragma once

#include "core/JSValue.h"

JSValue nativeRectAreaLightNodeModule();
JSValue nativeRectAreaLightNodeBinding(const std::string& name);
JSValue nativeRectAreaLightNodeConstructor();

class RectAreaLightNode {
    JSValue object_;
  public:
    explicit RectAreaLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeRectAreaLightNodeConstructor(); }
};

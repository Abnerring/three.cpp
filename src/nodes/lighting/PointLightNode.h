#pragma once

#include "core/JSValue.h"

JSValue nativePointLightNodeModule();
JSValue nativePointLightNodeBinding(const std::string& name);
JSValue nativePointLightNodeConstructor();
JSValue nativeDirectPointLight();

class PointLightNode {
    JSValue object_;
  public:
    explicit PointLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativePointLightNodeConstructor(); }
};

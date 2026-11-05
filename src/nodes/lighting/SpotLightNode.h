#pragma once

#include "core/JSValue.h"

JSValue nativeSpotLightNodeModule();
JSValue nativeSpotLightNodeBinding(const std::string& name);
JSValue nativeSpotLightNodeConstructor();

class SpotLightNode {
    JSValue object_;
  public:
    explicit SpotLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeSpotLightNodeConstructor(); }
};

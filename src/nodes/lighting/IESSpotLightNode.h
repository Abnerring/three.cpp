#pragma once

#include "core/JSValue.h"

JSValue nativeIESSpotLightNodeModule();
JSValue nativeIESSpotLightNodeBinding(const std::string& name);
JSValue nativeIESSpotLightNodeConstructor();

class IESSpotLightNode {
    JSValue object_;
  public:
    explicit IESSpotLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeIESSpotLightNodeConstructor(); }
};

#pragma once

#include "core/JSValue.h"

JSValue nativeLightProbeNodeConstructor();
JSValue nativeLightProbeNodeModule();
JSValue nativeLightProbeNodeBinding(const std::string& name);

class LightProbeNode {
    JSValue object_;
  public:
    explicit LightProbeNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeLightProbeNodeBinding("default"); }
};

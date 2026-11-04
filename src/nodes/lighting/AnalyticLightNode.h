#pragma once

#include "core/JSValue.h"

JSValue nativeAnalyticLightNodeConstructor();
JSValue nativeAnalyticLightNodeModule();
JSValue nativeAnalyticLightNodeBinding(const std::string& name);

class AnalyticLightNode {
    JSValue object_;
  public:
    explicit AnalyticLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeAnalyticLightNodeBinding("default"); }
};

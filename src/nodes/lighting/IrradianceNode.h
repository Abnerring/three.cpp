#pragma once
#include "core/JSValue.h"
JSValue nativeIrradianceNodeConstructor();
JSValue nativeIrradianceNodeModule();
JSValue nativeIrradianceNodeBinding(const std::string& name);
class IrradianceNode {
    JSValue object_;
  public:
    explicit IrradianceNode(const JSValue& node = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeIrradianceNodeBinding("default"); }
};

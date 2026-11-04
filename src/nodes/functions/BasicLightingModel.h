#pragma once

#include "core/JSValue.h"
#include <string>

// Native module for src/nodes/functions/BasicLightingModel.js at the pinned three.js revision.
JSValue nativeBasicLightingModelModule();
JSValue nativeBasicLightingModelBinding(const std::string& name);
JSValue nativeBasicLightingModel();

class BasicLightingModel {
    JSValue object_;
  public:
    BasicLightingModel();
    explicit BasicLightingModel(const JSValue& object) : object_(object) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeBasicLightingModel(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};

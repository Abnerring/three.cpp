#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeReferenceBaseNodeConstructor();
JSValue nativeReferenceBaseNodeModule();
JSValue nativeReferenceBaseNodeBinding(const std::string& name);

class ReferenceBaseNode {
    JSValue object_;
  public:
    explicit ReferenceBaseNode(const JSValue& property = {}, const JSValue& uniformType = {},
                               const JSValue& object = nullptr, const JSValue& count = nullptr)
        : object_(constructor().construct({property,uniformType,object,count})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeReferenceBaseNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
};

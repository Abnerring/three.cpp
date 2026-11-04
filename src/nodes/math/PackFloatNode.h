#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativePackFloatNodeModule();
JSValue nativePackFloatNodeBinding(const std::string& name);
JSValue nativePackFloatNodeConstructor();

class PackFloatNode {
    JSValue object_;

public:
    explicit PackFloatNode(const JSValue& encoding = {}, const JSValue& vectorNode = {})
        : object_(nativePackFloatNodeConstructor().construct({encoding, vectorNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativePackFloatNodeConstructor(); }
};

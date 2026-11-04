#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativePacked4x8IntegerNodeModule();
JSValue nativePacked4x8IntegerNodeBinding(const std::string& name);
JSValue nativePacked4x8IntegerNodeConstructor();

class Packed4x8IntegerNode {
    JSValue object_;

public:
    explicit Packed4x8IntegerNode(const JSValue& method = {}, const JSValue& aNode = {},
                                  const JSValue& bNode = nullptr)
        : object_(nativePacked4x8IntegerNodeConstructor().construct({method, aNode, bNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativePacked4x8IntegerNodeConstructor(); }
};

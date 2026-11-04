#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeBitcountNodeModule();
JSValue nativeBitcountNodeBinding(const std::string& name);
JSValue nativeBitcountNodeConstructor();

class BitcountNode {
    JSValue object_;

public:
    explicit BitcountNode(const JSValue& method = {}, const JSValue& aNode = {})
        : object_(nativeBitcountNodeConstructor().construct({method, aNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeBitcountNodeConstructor(); }
};

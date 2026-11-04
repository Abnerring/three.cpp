#pragma once

#include "core/JSValue.h"

#include <string>

JSValue nativeBitcastNodeModule();
JSValue nativeBitcastNodeBinding(const std::string& name);
JSValue nativeBitcastNodeConstructor();

class BitcastNode {
    JSValue object_;

public:
    explicit BitcastNode(const JSValue& valueNode = {}, const JSValue& conversionType = {},
                         const JSValue& inputType = nullptr)
        : object_(nativeBitcastNodeConstructor().construct({valueNode, conversionType, inputType})) {}

    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeBitcastNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};

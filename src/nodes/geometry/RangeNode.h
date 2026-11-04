#pragma once

#include "core/JSValue.h"

#include <string>

JSValue nativeRangeNodeModule();
JSValue nativeRangeNodeBinding(const std::string& name);
JSValue nativeRangeNodeConstructor();

class RangeNode {
    JSValue object_;

public:
    explicit RangeNode(const JSValue& minNode = {}, const JSValue& maxNode = {})
        : object_(nativeRangeNodeConstructor().construct({minNode, maxNode})) {}

    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeRangeNodeConstructor(); }
};

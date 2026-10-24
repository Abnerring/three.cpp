#pragma once
#include "core/JSMember.h"

JSValue nativeWGSLNodeParserConstructor();
class WGSLNodeParser {
    JSValue object_;
  public:
    WGSLNodeParser();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeWGSLNodeParserConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue parseFunction(const JSValue& source = {}) const;
};

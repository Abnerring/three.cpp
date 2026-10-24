#pragma once
#include "core/JSMember.h"

JSValue nativeGLSLNodeParserConstructor();
class GLSLNodeParser {
    JSValue object_;
  public:
    GLSLNodeParser();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeGLSLNodeParserConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue parseFunction(const JSValue& source = {}) const;
};

#pragma once
#include "core/JSMember.h"

JSValue nativeNodeParserConstructor();
class NodeParser {
    JSValue object_;
  public:
    NodeParser();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeNodeParserConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue parseFunction(const JSValue& source = {}) const;
    static std::string abstractWarning() { return "Abstract function."; }
    static bool returnsNothing() { return true; }
    static bool throwsOnMissingImplementation() { return false; }
};

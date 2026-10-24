#pragma once
#include "core/JSValue.h"

// NodeBuilder.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Mutable state and method dispatch live on the ordinary runtime object.
class NodeBuilder {
  public:
    NodeBuilder(JSValue object, JSValue renderer, JSValue parser);
    JSValue object() const;
    static JSValue constructor();
    static JSValue prototype();
  private:
    JSValue object_;
};
JSValue nativeNodeBuilderConstructor();

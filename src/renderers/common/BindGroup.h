#pragma once
#include "core/JSValue.h"

// BindGroup.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Bindings retain their ordinary runtime identities; mutable fields are live.
JSValue nativeBindGroupConstructor();
class BindGroup {
    JSValue object_;
  public:
    explicit BindGroup(const JSValue& name = {}, const JSValue& bindings = {});
    JSValue object() const;
    static JSValue constructor();
    static JSValue prototype();
};

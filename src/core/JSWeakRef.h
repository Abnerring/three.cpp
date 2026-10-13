#pragma once
#include "core/JSValue.h"

// Object, array, and function targets. Symbol targets and @@toStringTag require
// the runtime's still-unimplemented Symbol primitive and keys.
class JSWeakRef {
    JSValue object_;
  public:
    explicit JSWeakRef(const JSValue& object);
    JSValue object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
    JSValue deref() const;
    static bool isRef(const JSValue& value);
    static JSValue create(const JSValue& prototype, const JSValue& target);
};
JSValue nativeWeakRefConstructor();
JSValue createNativeWeakRef(const JSValue& target);
bool isNativeWeakRef(const JSValue& value);

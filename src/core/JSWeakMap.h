#pragma once
#include "core/JSValue.h"

// Native WeakMap for the runtime's object, array, and function keys. Conditional
// entries live in JSHeap records and participate in ephemeron fixed-point GC.
// Symbol keys, @@toStringTag, and iterable constructor initialization require
// the still-unimplemented Symbol/iterator runtime and are not emulated here.
class JSWeakMap {
    JSValue object_;
  public:
    JSWeakMap();
    explicit JSWeakMap(const JSValue& object);
    JSValue object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
    operator JSValue() const { return object(); }
    bool has(const JSValue& key) const;
    JSValue get(const JSValue& key) const;
    bool remove(const JSValue& key);
    JSValue set(const JSValue& key, const JSValue& value);
    static bool isMap(const JSValue& value);
    static JSValue create(const JSValue& prototype);
};
JSValue nativeWeakMapConstructor();
JSValue createNativeWeakMap();
bool isNativeWeakMap(const JSValue& value);

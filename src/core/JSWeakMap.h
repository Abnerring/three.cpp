#pragma once
#include "core/JSValue.h"

// Native WeakMap for object, array, function and unregistered Symbol keys.
// Conditional entries participate in ephemeron fixed-point GC. The Symbol
// registry, @@toStringTag and iterable constructor initialization remain pending.
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

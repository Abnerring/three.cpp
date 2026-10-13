#pragma once
#include "core/JSValue.h"

// Native weak targets/tokens and strong cleanup callback/held-value graph edges.
// Cleanup timing is host-defined. Symbol targets/tokens and @@toStringTag await
// the runtime's Symbol support; cleanupSome is not a standard method.
class JSFinalizationRegistry {
    JSValue object_;
  public:
    explicit JSFinalizationRegistry(const JSValue& object);
    JSValue object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
    void registerTarget(const JSValue& target, const JSValue& heldValue, const JSValue& token = {});
    bool unregister(const JSValue& token);
    static bool isRegistry(const JSValue& value);
    static JSValue create(const JSValue& prototype, const JSValue& callback);
    // Host cleanup-task entry, never exposed as a JavaScript property.
    static void cleanup(const JSValue& registry);
};
JSValue nativeFinalizationRegistryConstructor();
JSValue createNativeFinalizationRegistry(const JSValue& callback);
bool isNativeFinalizationRegistry(const JSValue& value);

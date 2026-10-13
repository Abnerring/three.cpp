#pragma once
#include "core/JSValue.h"

// Native lazy-module reentry is diagnosed instead of returning an incomplete
// export or recursing. This is a host initialization contract, not an ESM loader
// or a complete implementation of ECMAScript live bindings / temporal dead zones.
class JSModuleInitializationError : public std::logic_error {
  public:
    explicit JSModuleInitializationError(const std::string& name)
        : std::logic_error("Reentrant initialization of native module " + name) {}
};
class JSModuleInitializationScope {
    bool& active_;

  public:
    JSModuleInitializationScope(bool& active, const char* name) : active_(active) {
        if (active_)
            throw JSModuleInitializationError(name);
        active_ = true;
    }
    ~JSModuleInitializationScope() { active_ = false; }
    JSModuleInitializationScope(const JSModuleInitializationScope&) = delete;
    JSModuleInitializationScope& operator=(const JSModuleInitializationScope&) = delete;
};

// Non-owning resolver for a realm-local module export. Copying this facade does
// not retain a heap: use an explicit JSValue to hold the current export object.
class JSModuleObject {
    using Resolver = JSValue (*)();
    Resolver resolve_;
    static void requireRealm() {
        if (!JSValue::hasActiveHeap())
            throw JSTypeError("Module mutation requires a live JSValue realm or module handle");
    }
    static void validate(const JSValue& value) { static_cast<void>(JSWeakValue(value)); }
    static void validate(const JSPropertyDescriptor& descriptor) {
        if (descriptor.value)
            validate(*descriptor.value);
        if (descriptor.getter)
            validate(*descriptor.getter);
        if (descriptor.setter)
            validate(*descriptor.setter);
    }

  public:
    constexpr explicit JSModuleObject(Resolver resolve) : resolve_(resolve) {}
    JSValue object() const { return resolve_(); }
    operator JSValue() const { return object(); }
    JSValue get(const std::string& key) const { return object().get(key); }
    JSValue get(const std::string& key, const JSValue& receiver) const {
        validate(receiver);
        return object().get(key, receiver);
    }
    void set(const std::string& key, const JSValue& value) const {
        validate(value);
        requireRealm();
        object().set(key, value);
    }
    bool reflectSet(const std::string& key, const JSValue& value, const JSValue& receiver) const {
        validate(value);
        validate(receiver);
        requireRealm();
        return object().reflectSet(key, value, receiver);
    }
    bool hasOwn(const std::string& key) const { return object().hasOwn(key); }
    bool has(const std::string& key) const { return object().has(key); }
    bool erase(const std::string& key) const {
        requireRealm();
        return object().erase(key);
    }
    std::vector<std::string> keys() const { return object().keys(); }
    std::vector<std::string> ownPropertyNames() const { return object().ownPropertyNames(); }
    std::optional<JSPropertyDescriptor> getOwnPropertyDescriptor(const std::string& key) const {
        return object().getOwnPropertyDescriptor(key);
    }
    bool defineOwnProperty(const std::string& key, const JSPropertyDescriptor& descriptor) const {
        validate(descriptor);
        requireRealm();
        return object().defineOwnProperty(key, descriptor);
    }
    void defineProperty(const std::string& key, const JSPropertyDescriptor& descriptor) const {
        validate(descriptor);
        requireRealm();
        object().defineProperty(key, descriptor);
    }
    JSValue getPrototype() const { return object().getPrototype(); }
    bool setPrototype(const JSValue& prototype) const {
        validate(prototype);
        requireRealm();
        return object().setPrototype(prototype);
    }
    void preventExtensions() const {
        requireRealm();
        object().preventExtensions();
    }
    bool isExtensible() const { return object().isExtensible(); }
    bool isObject() const { return object().isObject(); }
    bool sameValue(const JSValue& other) const {
        validate(other);
        return object().sameValue(other);
    }
    bool strictEquals(const JSValue& other) const {
        validate(other);
        return object().strictEquals(other);
    }
    std::optional<JsonValue> toJSON() const { return object().toJSON(); }
    JsonValue trace() const { return object().trace(); }
    std::size_t collectGarbage() const { return object().collectGarbage(); }
    std::size_t heapObjectCount() const { return object().heapObjectCount(); }
};

#pragma once
#include "core/JSValue.h"

// Heap-owned strong collection edges; wrapper handles remain external GC roots.
class JSSet {
    JSValue object_;
  public:
    JSSet();
    explicit JSSet(const JSValue& object);
    JSSet(const JSSet& other) : object_(other.object()) {}
    JSSet(JSSet&& other) : object_(other.object()) { other.object_={}; }
    JSSet& operator=(const JSSet& other) { static_cast<void>(JSWeakValue(object_)); object_=other.object(); return *this; }
    JSSet& operator=(JSSet&& other) { if(this!=&other) { static_cast<void>(JSWeakValue(object_)); object_=other.object(); other.object_={}; } return *this; }
    JSValue object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
    operator JSValue() const { return object(); }
    std::size_t size() const;
    void clear();
    bool has(const JSValue& key) const;
    bool remove(const JSValue& key);
    JSValue add(const JSValue& value);
    JSValue keys() const;
    JSValue values() const;
    JSValue entries() const;
    void forEach(const JSValue& callback, const JSValue& thisArg = {}) const;
    static bool isCollection(const JSValue& value);
    static JSValue create(const JSValue& prototype);
};
JSValue nativeSetConstructor();
JSValue createNativeSet();
bool isNativeSet(const JSValue& value);

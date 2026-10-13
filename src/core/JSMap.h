#pragma once
#include "core/JSValue.h"

// Heap-owned strong collection edges; wrapper handles remain external GC roots.
class JSMap {
    JSValue object_;
  public:
    JSMap();
    explicit JSMap(const JSValue& object);
    JSMap(const JSMap& other) : object_(other.object()) {}
    JSMap(JSMap&& other) : object_(other.object()) { other.object_={}; }
    JSMap& operator=(const JSMap& other) { static_cast<void>(JSWeakValue(object_)); object_=other.object(); return *this; }
    JSMap& operator=(JSMap&& other) { if(this!=&other) { static_cast<void>(JSWeakValue(object_)); object_=other.object(); other.object_={}; } return *this; }
    JSValue object() const { static_cast<void>(JSWeakValue(object_)); return object_; }
    operator JSValue() const { return object(); }
    std::size_t size() const;
    void clear();
    bool has(const JSValue& key) const;
    bool remove(const JSValue& key);
    JSValue get(const JSValue& key) const;
    JSValue set(const JSValue& key, const JSValue& value);
    JSValue keys() const;
    JSValue values() const;
    JSValue entries() const;
    void forEach(const JSValue& callback, const JSValue& thisArg = {}) const;
    static bool isCollection(const JSValue& value);
    static JSValue create(const JSValue& prototype);
};
JSValue nativeMapConstructor();
JSValue createNativeMap();
bool isNativeMap(const JSValue& value);

#pragma once
#include "core/JSValue.h"

// An assignable field view. The containing handle owns the graph root; this
// member neither duplicates storage nor contributes a hidden GC root.
class JSMember {
    JSValue* object_;
    const char* key_;
  public:
    JSMember(JSValue* object, const char* key) : object_(object), key_(key) {}
    JSValue value() const { return object_->get(key_); }
    operator JSValue() const { return value(); }
    JSMember& operator=(const JSValue& value) { object_->set(key_, value); return *this; }
    JSMember& operator=(const JSMember& other) { return *this = other.value(); }
    double toNumber() const { return value().toNumber(); }
    bool toBoolean() const { return value().toBoolean(); }
    std::string toString() const { return value().toString(); }
    bool isUndefined() const { return value().isUndefined(); }
    bool isNull() const { return value().isNull(); }
    bool strictEquals(const JSValue& other) const { return value().strictEquals(other); }
    JSValue get(const std::string& key) const { return value().get(key); }
    void set(const std::string& key, const JSValue& data) { auto object = value(); object.set(key,data); }
};

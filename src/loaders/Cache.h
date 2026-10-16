#pragma once
#include "core/JSModuleObject.h"

JSValue nativeCache();

class CacheStore {
  public:
    constexpr CacheStore() = default;
    CacheStore(const CacheStore&) = delete;
    CacheStore& operator=(const CacheStore&) = delete;
    JSModuleMember enabled{nativeCache, "enabled"}, files{nativeCache, "files"};
    JSValue object() const { return nativeCache(); }
    void add(const JSValue& key, const JSValue& file);
    JSValue get(const JSValue& key) const;
    void remove(const JSValue& key);
    void clear();
};
extern CacheStore Cache;

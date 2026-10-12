#pragma once
#include "core/JSMember.h"

class CacheStore {
  public:
    CacheStore();
    CacheStore(const CacheStore&)=delete;
    CacheStore& operator=(const CacheStore&)=delete;
    JSMember enabled{&object_,"enabled"},files{&object_,"files"};
    JSValue object() const {return object_;}
    void add(JSValue key,JSValue file);
    JSValue get(JSValue key) const;
    void remove(JSValue key);
    void clear();
  private:
    JSValue object_;
};
extern thread_local CacheStore Cache;

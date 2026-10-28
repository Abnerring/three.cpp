#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeStorageBufferAttributeConstructor();
JSValue nativeStorageBufferAttributeModule();
JSValue nativeStorageBufferAttributeBinding(const std::string& name);
class StorageBufferAttribute {
    JSValue object_;
  public:
    explicit StorageBufferAttribute(const JSValue& count={},const JSValue& itemSize={},const JSValue& typeClass={}):object_(constructor().construct({count,itemSize,typeClass})){}
    static JSValue constructor(){return nativeStorageBufferAttributeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeStorageInstancedBufferAttributeConstructor();
JSValue nativeStorageInstancedBufferAttributeModule();
JSValue nativeStorageInstancedBufferAttributeBinding(const std::string& name);
class StorageInstancedBufferAttribute {
    JSValue object_;
  public:
    explicit StorageInstancedBufferAttribute(const JSValue& count={},const JSValue& itemSize={},const JSValue& typeClass={}):object_(constructor().construct({count,itemSize,typeClass})){}
    static JSValue constructor(){return nativeStorageInstancedBufferAttributeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

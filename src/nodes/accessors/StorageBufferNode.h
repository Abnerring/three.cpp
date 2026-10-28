#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeStorageBufferNodeConstructor();
JSValue nativeStorageBufferNodeModule();
JSValue nativeStorageBufferNodeBinding(const std::string& name);
class StorageBufferNode {
    JSValue object_;
  public:
    explicit StorageBufferNode(const JSValue& value={},const JSValue& type=nullptr,const JSValue& count=0):object_(constructor().construct({value,type,count})){}
    static JSValue constructor(){return nativeStorageBufferNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

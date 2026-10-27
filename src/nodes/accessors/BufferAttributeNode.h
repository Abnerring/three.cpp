#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeBufferAttributeNodeConstructor();
JSValue nativeBufferAttributeNodeModule();
JSValue nativeBufferAttributeNodeBinding(const std::string& name);

class BufferAttributeNode {
    JSValue object_;
public:
    explicit BufferAttributeNode(const JSValue& value={},const JSValue& bufferType=nullptr,const JSValue& bufferStride=0,const JSValue& bufferOffset=0);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeBufferAttributeNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getHash(const JSValue& builder)const;
    JSValue generateNodeType(const JSValue& builder)const;
    JSValue setup(const JSValue& builder)const;
    JSValue generate(const JSValue& builder)const;
    JSValue getInputType()const;
    JSValue setUsage(const JSValue& value)const;
    JSValue setInstanced(const JSValue& value)const;
};

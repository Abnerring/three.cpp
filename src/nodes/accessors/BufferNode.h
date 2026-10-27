#pragma once
#include "core/JSMember.h"
JSValue nativeBufferNodeConstructor();
JSValue nativeBufferNodeModule();
JSValue nativeBufferNodeBinding(const std::string& name);
class BufferNode {
    JSValue object_;
public:
    BufferNode(const JSValue& value={},const JSValue& bufferType={},const JSValue& bufferCount=0);
    BufferNode(const BufferNode& other):object_(other.object_){}
    BufferNode& operator=(const BufferNode& other){object_=other.object_;return *this;}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeBufferNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSMember value{&object_,"value"},bufferType{&object_,"bufferType"},bufferCount{&object_,"bufferCount"},updateRanges{&object_,"updateRanges"};
    JSValue addUpdateRange(const JSValue& start,const JSValue& count)const;
    JSValue clearUpdateRanges()const;
    JSValue getElementType(const JSValue& builder)const;
    JSValue getInputType(const JSValue& builder={})const;
};

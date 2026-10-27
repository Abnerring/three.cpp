#pragma once
#include "core/JSMember.h"
JSValue nativeUniformArrayNodeConstructor();
JSValue nativeUniformArrayNodeModule();
JSValue nativeUniformArrayNodeBinding(const std::string& name);
JSValue nativeUniformArrayElementNodeConstructor();
class UniformArrayNode {
    JSValue object_;
public:
    UniformArrayNode(const JSValue& array={},const JSValue& elementType=nullptr);
    UniformArrayNode(const UniformArrayNode& other):object_(other.object_){}
    UniformArrayNode& operator=(const UniformArrayNode& other){object_=other.object_;return *this;}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeUniformArrayNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSMember array{&object_,"array"},value{&object_,"value"},elementType{&object_,"elementType"},bufferCount{&object_,"bufferCount"},bufferType{&object_,"bufferType"};
    JSValue getPaddedType(const JSValue& builder)const;
    JSValue updateBuffer()const;
    JSValue update(const JSValue& frame={})const;
    JSValue onUpdate(const JSValue& callback,const JSValue& updateType={})const;
    JSValue setup(const JSValue& builder)const;
    JSValue element(const JSValue& index)const;
};

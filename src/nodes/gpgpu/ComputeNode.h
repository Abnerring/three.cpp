#pragma once
#include "core/JSMember.h"
JSValue nativeComputeNodeConstructor();
JSValue nativeComputeNodeModule();
JSValue nativeComputeNodeBinding(const std::string& name);
class ComputeNode {
    JSValue object_;
public:
    ComputeNode(const JSValue& computeNode={},const JSValue& workgroupSize={});
    ComputeNode(const ComputeNode& other):object_(other.object_){}
    ComputeNode& operator=(const ComputeNode& other){object_=other.object_;return *this;}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeComputeNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSMember computeNode{&object_,"computeNode"},workgroupSize{&object_,"workgroupSize"},count{&object_,"count"},dispatchSize{&object_,"dispatchSize"},version{&object_,"version"},name{&object_,"name"},onInitFunction{&object_,"onInitFunction"},countNode{&object_,"countNode"},updateBeforeType{&object_,"updateBeforeType"};
    JSValue setName(const JSValue& name)const;
    JSValue label(const JSValue& name)const;
    JSValue onInit(const JSValue& callback)const;
    JSValue getUpdateBeforeType(const JSValue& frame)const;
    JSValue updateBefore(const JSValue& frame)const;
    JSValue setup(const JSValue& builder)const;
    JSValue generate(const JSValue& builder,const JSValue& output={})const;
};

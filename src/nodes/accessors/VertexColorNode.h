#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeVertexColorNodeConstructor();
JSValue nativeVertexColorNodeModule();
JSValue nativeVertexColorNodeBinding(const std::string& name);
class VertexColorNode {
    JSValue object_;
public:
    explicit VertexColorNode(const JSValue& index={}):object_(constructor().construct({index})){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeVertexColorNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getAttributeName(const JSValue& builder={})const{return object_.get("getAttributeName").call(object_,{builder});}
    JSValue generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
    JSValue serialize(const JSValue& data)const{return object_.get("serialize").call(object_,{data});}
    JSValue deserialize(const JSValue& data)const{return object_.get("deserialize").call(object_,{data});}
};

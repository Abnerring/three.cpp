#pragma once
#include "core/JSMember.h"
JSValue nativeExpressionNodeConstructor();
JSValue nativeExpressionNodeModule();
JSValue nativeExpressionNodeBinding(const std::string& name);
class ExpressionNode {
    JSValue object_;
public:
    ExpressionNode(const JSValue& snippet="",const JSValue& nodeType="void");
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeExpressionNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue generate(const JSValue& builder,const JSValue& output={})const;
    bool isCacheable(const JSValue& builder={})const{return object_.get("isCacheable").call(object_,{builder}).toBoolean();}
};

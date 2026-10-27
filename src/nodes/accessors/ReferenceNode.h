#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeReferenceNodeConstructor();
JSValue nativeReferenceNodeModule();
JSValue nativeReferenceNodeBinding(const std::string& name);
class ReferenceNode {
    JSValue object_;
  public:
    explicit ReferenceNode(const JSValue& property={},const JSValue& type={},const JSValue& object=nullptr,const JSValue& count=nullptr)
        :object_(constructor().construct({property,type,object,count})){}
    static JSValue constructor(){return nativeReferenceNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

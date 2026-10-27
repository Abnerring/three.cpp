#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeMaterialReferenceNodeConstructor();
JSValue nativeMaterialReferenceNodeModule();
JSValue nativeMaterialReferenceNodeBinding(const std::string& name);
class MaterialReferenceNode {
    JSValue object_;
  public:
    explicit MaterialReferenceNode(const JSValue& property={},const JSValue& type={},const JSValue& material=nullptr):object_(constructor().construct({property,type,material})){}
    static JSValue constructor(){return nativeMaterialReferenceNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

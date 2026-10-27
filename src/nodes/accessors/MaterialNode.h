#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeMaterialNodeConstructor();
JSValue nativeMaterialNodeModule();
JSValue nativeMaterialNodeBinding(const std::string& name);
class MaterialNode {
    JSValue object_;
  public:
    explicit MaterialNode(const JSValue& scope={}):object_(constructor().construct({scope})){}
    static JSValue constructor(){return nativeMaterialNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
};

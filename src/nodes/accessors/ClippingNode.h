#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeClippingNodeConstructor();
JSValue nativeClippingNodeModule();
JSValue nativeClippingNodeBinding(const std::string& name);
class ClippingNode {
    JSValue object_;
public:
    explicit ClippingNode(const JSValue& scope={}):object_(constructor().construct({scope})){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeClippingNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
    JSValue setupAlphaToCoverage(const JSValue& intersection,const JSValue& planes)const{return object_.get("setupAlphaToCoverage").call(object_,{intersection,planes});}
    JSValue setupDefault(const JSValue& intersection,const JSValue& planes)const{return object_.get("setupDefault").call(object_,{intersection,planes});}
    JSValue setupHardwareClipping(const JSValue& planes,const JSValue& builder)const{return object_.get("setupHardwareClipping").call(object_,{planes,builder});}
};

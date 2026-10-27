#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeInspectorBaseConstructor();
JSValue nativeInspectorBaseModule();
JSValue nativeInspectorBaseBinding(const std::string& name);

class InspectorBase {
    JSValue object_;
public:
    InspectorBase():object_(constructor().construct()){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeInspectorBaseBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue setRenderer(const JSValue& renderer)const{return object_.get("setRenderer").call(object_,{renderer});}
    JSValue getRenderer()const{return object_.get("getRenderer").call(object_);}
    JSValue nodeFrame()const{return object_.get("nodeFrame");}
    void begin()const{object_.get("begin").call(object_);}
    void finish()const{object_.get("finish").call(object_);}
    void dispose()const{object_.get("dispose").call(object_);}
};

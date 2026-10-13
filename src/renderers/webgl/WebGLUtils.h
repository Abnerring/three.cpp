#pragma once
#include "core/JSValue.h"
// Pinned format protocol over injected commands, not Vulkan format discovery.
JSValue nativeWebGLUtilsConstructor();
class WebGLUtils {
    JSValue object_;
    static JSValue copy(const JSValue& value){static_cast<void>(JSWeakValue(value));return value;}
  public:
    explicit WebGLUtils(const JSValue& gl={},const JSValue& extensions={});
    WebGLUtils(const WebGLUtils& other):object_(copy(other.object_)){}
    WebGLUtils(WebGLUtils&& other):object_(copy(other.object_)){other.object_={};}
    WebGLUtils& operator=(const WebGLUtils& other){static_cast<void>(JSWeakValue(object_));object_=copy(other.object_);return *this;}
    WebGLUtils& operator=(WebGLUtils&& other){static_cast<void>(JSWeakValue(object_));if(this!=&other){object_=copy(other.object_);other.object_={};}return *this;}
    JSValue object()const{return copy(object_);}
    operator JSValue()const{return object();}
    JSValue convert(const JSValue& p={},const JSValue& colorSpace={});
};

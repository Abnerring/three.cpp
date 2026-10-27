#pragma once
#include "core/JSValue.h"
JSValue nativeTextureSizeNodeConstructor();
JSValue nativeTextureSizeNodeModule();
JSValue nativeTextureSizeNodeBinding(const std::string& name);
class TextureSizeNode {
    JSValue object_;
public:
    TextureSizeNode(const JSValue& textureNode={},const JSValue& levelNode=nullptr);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeTextureSizeNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue generate(const JSValue& builder)const;
};

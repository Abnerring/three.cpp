#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeCubeTextureNodeConstructor();
JSValue nativeCubeTextureNodeModule();
JSValue nativeCubeTextureNodeBinding(const std::string& name);
class CubeTextureNode {
    JSValue object_;
public:
    explicit CubeTextureNode(const JSValue& value={},const JSValue& uvNode=nullptr,const JSValue& levelNode=nullptr,const JSValue& biasNode=nullptr);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeCubeTextureNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getInputType()const;
    JSValue getDefaultUV()const;
    void setUpdateMatrix(const JSValue& value)const;
    JSValue setupUV(const JSValue& builder,const JSValue& uvNode)const;
    JSValue generateUV(const JSValue& builder,const JSValue& cubeUV)const;
};

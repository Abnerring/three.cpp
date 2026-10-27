#pragma once
#include "core/JSValue.h"
JSValue nativeBuiltinNodeConstructor();
JSValue nativeBuiltinNodeModule();
JSValue nativeBuiltinNodeBinding(const std::string& name);
class BuiltinNode {
    JSValue object_;
    mutable std::string nameCache_;
public:
    static constexpr bool isBuiltinNode=true;
    static constexpr const char* kNodeType="float";
    explicit BuiltinNode(const JSValue& name={});
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeBuiltinNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    const std::string& name()const;
    static bool isCacheable();
    std::string generate()const;
    JSValue generate(const JSValue& builder)const;
};

#pragma once
#include "core/JSValue.h"
#include <string>
#include <vector>

JSValue nativeModelNodeConstructor();
JSValue nativeModelNodeModule();
JSValue nativeModelNodeBinding(const std::string& name);

class ModelNode {
    JSValue object_;
public:
    explicit ModelNode(const JSValue& scope={}):object_(constructor().construct({scope})){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeModelNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    static constexpr const char* kTypeName="ModelNode";
    static const std::vector<std::string>& scopes();
    static bool isScope(const std::string& name);
    static constexpr bool holdsOwnObject=false;
};

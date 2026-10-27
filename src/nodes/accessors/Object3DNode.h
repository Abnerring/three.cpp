#pragma once
#include "core/JSValue.h"
#include <string>
#include <vector>

JSValue nativeObject3DNodeConstructor();
JSValue nativeObject3DNodeModule();
JSValue nativeObject3DNodeBinding(const std::string& name);

class Object3DNode {
    JSValue object_;
public:
    explicit Object3DNode(const JSValue& scope={},const JSValue& object3d=nullptr)
        :object_(constructor().construct({scope,object3d})){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeObject3DNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    static constexpr const char* kTypeName="Object3DNode";
    // Compatibility helpers return an empty string for the JS undefined type.
    static const std::vector<std::string>& scopes();
    static std::string nodeType(const std::string& scope);
    static bool isCacheable(){return false;}
    static constexpr const char* kUpdateType="OBJECT";
};

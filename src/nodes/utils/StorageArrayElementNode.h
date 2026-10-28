#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeStorageArrayElementNodeConstructor();
JSValue nativeStorageArrayElementNodeModule();
JSValue nativeStorageArrayElementNodeBinding(const std::string& name);
class StorageArrayElementNode {
    JSValue object_;
  public:
    explicit StorageArrayElementNode(const JSValue& node={},const JSValue& index={}):object_(constructor().construct({node,index})){}
    static JSValue constructor(){return nativeStorageArrayElementNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static constexpr const char* kTypeName="StorageArrayElementNode";
    static constexpr const char* kMissingMemberType="void";
    static constexpr const char* kMemberNodeMissingType="float";
    static std::string memberType(bool hasStructType,const std::string& structAnswer);
    static bool familiesAgreeOnMissingMember();
    static bool formats(bool isAssignContext);
    static bool usesPboFallback(bool storageBufferAvailable,bool isPbo,bool isAssignContext,bool valueIsInstancedBufferAttribute,bool isComputeStage);
};

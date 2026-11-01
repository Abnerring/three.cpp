// Native MeshBasicMaterial source class. Typed host implementation stays in MeshBasicMaterial.cpp.
#include "materials/MeshBasicMaterial.h"
#include "math/Euler.h"
#include "core/JSModuleObject.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned index){return index<args.size()?args[index]:JSValue{};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
#include "MeshBasicMaterialRuntimeStatements.inc"
#include "MeshBasicMaterialSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshBasicMaterialConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;
    static thread_local bool initializing=false;
    JSModuleInitializationScope initialization(initializing,"MeshBasicMaterial");
    // Resolve imports once; mutable exported facade properties are not lexical imports.
    const auto base=nativeMaterialConstructor();
    auto imports=JSValue::object(nullptr);
    own(imports,"Color",nativeColorConstructor());
    own(imports,"Euler",nativeEulerConstructor());
    auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto holder=captures[0];
        // Derived constructors use their current [[Prototype]] and preserve new.target.
        auto self=holder.get("constructor").getPrototype().construct({},target);
        return meshBasicMaterial_constructor(self,arg(args,0),holder.get("imports"));
    },{holder},"MeshBasicMaterial",1);
    own(holder,"constructor",constructor);
    constructor.setPrototype(base);
    auto prototype=constructor.get("prototype");prototype.setPrototype(base.get("prototype"));
    constructor.setFunctionSourceText(MeshBasicMaterialSource::MeshBasicMaterial);
    auto copy=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{
        return meshBasicMaterial_copy(self,arg(args,0),captures[0]);
    },{prototype},"copy",1);
    copy.setFunctionSourceText(MeshBasicMaterialSource::copy);
    JSPropertyDescriptor method;method.value=copy;method.writable=method.configurable=true;
    prototype.defineProperty("copy",method);
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeMeshBasicMaterialModule(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    auto bindings=JSValue::object(nullptr);own(bindings,"MeshBasicMaterial",nativeMeshBasicMaterialConstructor());
    auto facade=JSValue::object();own(facade,"MeshBasicMaterial",bindings.get("MeshBasicMaterial"));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeMeshBasicMaterialBinding(const std::string& name){const auto keeper=nativeMeshBasicMaterialModule();return lexical().lock()->get(name);}

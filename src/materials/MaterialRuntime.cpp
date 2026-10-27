// Native Material class, parallel to the existing numeric Vulkan host adapter.
#include "materials/Material.h"
#include "math/Plane.h"
#include "math/MathUtils.h"
#include "core/JSModuleObject.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
#include "animation/AnimationUtils.h"
#include "utils.h"
#include <unordered_set>
#include <tuple>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
struct Invocation{JSValue object,method;JSValue operator()(const Args& args={})const{return method.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue add(const JSValue& a,const JSValue& b){const auto left=a.toPrimitive(),right=b.toPrimitive();if(left.type()==JSValue::Type::String||right.type()==JSValue::Type::String)return left.toString()+right.toString();return left.toNumber()+right.toNumber();}
bool greater(const JSValue& a,const JSValue& b){const auto left=a.toPrimitive(JSValue::PreferredType::Number),right=b.toPrimitive(JSValue::PreferredType::Number);return left.toNumber()>right.toNumber();}
std::string typeOf(const JSValue& value){if(value.isCallable())return "function";if(value.isUndefined())return "undefined";if(value.isSymbol())return "symbol";if(value.type()==JSValue::Type::String)return "string";if(value.type()==JSValue::Type::Number)return "number";if(value.type()==JSValue::Type::Boolean)return "boolean";return "object";}
template<class F>void forIn(const JSValue& object,F body){
    if(object.isNull()||object.isUndefined())return;
    auto receiver=object;
    if(!receiver.isObject()){
        if(receiver.type()==JSValue::Type::String)receiver=createNativeStringObject(receiver);
        else if(receiver.type()==JSValue::Type::Number)receiver=createNativeNumberObject(receiver);
        else return; // Boolean/Symbol wrapper intrinsics remain outside this runtime.
    }
    // V8 snapshots candidates across the chain. Proxy descriptor checks belong
    // to the per-key filter, after collection and the preceding loop body.
    std::unordered_set<std::string> visited;std::vector<std::string> keys;
    auto level=receiver;
    while(!level.isNull()){
        const bool proxy=level.isProxy();
        for(const auto& key:level.ownPropertyNames()){
            if(visited.contains(key))continue;
            if(proxy){visited.insert(key);keys.push_back(key);}
            else{
                const auto d=level.getOwnPropertyDescriptor(key);
                if(!d)continue;
                visited.insert(key);if(d->enumerable.value_or(false))keys.push_back(key);
            }
        }
        level=level.getPrototype();
    }
    for(const auto& key:keys){
        auto owner=receiver;
        while(!owner.isNull()){
            const auto d=owner.getOwnPropertyDescriptor(key);
            // Ordinary properties remain candidates when their enumerable flag
            // changes. V8 applies the current flag only at a Proxy boundary.
            if(d){if(!owner.isProxy()||d->enumerable.value_or(false))body(key);break;}
            // Integer-indexed exotic lookup stops at an invalid numeric index;
            // it does not fall through to an inherited numeric property.
            if(owner.isHostObject("three.AnimationArray")&&
               (key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key))break;
            owner=owner.getPrototype();
        }
    }
}
JSValue defineMaterialId(JSValue self,JSValue state,const JSValue& imports){
    const auto define=invoke(imports.get("Object"),"defineProperty");
    const auto id=state.get("id").toNumber();state.set("id",id+1);
    auto descriptor=JSValue::object();own(descriptor,"value",id);
    return define({self,"id",descriptor});
}
JSValue extractFromCache(const JSValue& cache){
    auto values=JSValue::array();forIn(cache,[&](const std::string& key){
        auto value=cache.get(key);
        if(value.isNull()||value.isUndefined())throw JSTypeError("Cannot delete metadata of null or undefined");
        if(value.isObject()&&!value.erase("metadata"))throw JSTypeError("Cannot delete metadata");
        invoke(values,"push")({value});
    });return values;
}
#include "MaterialRuntimeStatements.inc"
#include "MaterialSource.inc"
JSValue copy(JSValue self,const JSValue& source,const JSValue& imports){
    const auto fields=[&](std::initializer_list<const char*> names){for(const auto* key:names)self.set(key,source.get(key));};
    fields({"name","blending","side","vertexColors","opacity","transparent","blendSrc","blendDst","blendEquation","blendSrcAlpha","blendDstAlpha","blendEquationAlpha"});
    invoke(self.get("blendColor"),"copy")({source.get("blendColor")});
    fields({"blendAlpha","depthFunc","depthTest","depthWrite","stencilWriteMask","stencilFunc","stencilRef","stencilFuncMask","stencilFail","stencilZFail","stencilZPass","stencilWrite"});
    const auto src=source.get("clippingPlanes");JSValue dst=nullptr;
    if(!src.isNull()){
        const auto n=src.get("length");dst=imports.get("Array").construct({n});
        for(double i=0;!JSValue(i).strictEquals(n);++i)dst.set(JSValue(i),invoke(src.get(JSValue(i)),"clone")());
    }
    self.set("clippingPlanes",dst);
    fields({"clipIntersection","clipShadows","shadowSide","colorWrite","precision","polygonOffset","polygonOffsetFactor","polygonOffsetUnits","dithering","alphaTest","alphaHash","alphaToCoverage","premultipliedAlpha","forceSinglePass","allowOverride","visible","toneMapped"});
    const auto json=Animation_detail::stringifyJSON(source.get("userData"));
    self.set("userData",Animation_detail::parseJSON(json.value_or("undefined")));return self;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto imports=captures[1];
    if(name=="get_alphaTest")return self.get("_alphaTest");
    if(name=="set_alphaTest"){
        const auto previous=self.get("_alphaTest").toNumber(),value=arg(args,0).toNumber();
        if((previous>0)!=(value>0))self.set("version",self.get("version").toNumber()+1);
        self.set("_alphaTest",arg(args,0));return {};
    }
    if(name=="set_needsUpdate"){if(arg(args,0).strictEquals(true))self.set("version",self.get("version").toNumber()+1);return {};}
    if(name=="onBeforeRender"||name=="onBeforeCompile")return {};
    if(name=="customProgramCacheKey")return invoke(self.get("onBeforeCompile"),"toString")();
    if(name=="setValues"){
        const auto values=arg(args,0);if(values.isUndefined())return {};
        forIn(values,[&](const std::string& key){
            const auto value=values.get(key);
            if(value.isUndefined()){ThreeUtils::warn({"Material: parameter '"+key+"' has value of undefined."});return;}
            const auto current=self.get(key);
            if(current.isUndefined()){ThreeUtils::warn({"Material: '"+key+"' is not a property of THREE."+self.get("type").toString()+"."});return;}
            if(current.toBoolean()&&current.get("isColor").toBoolean())invoke(current,"set")({value});
            else{
                const auto matches=[&](const char* flag){return current.toBoolean()&&current.get(flag).toBoolean()&&value.toBoolean()&&value.get(flag).toBoolean();};
                if(matches("isVector2")||matches("isEuler")||matches("isVector3"))invoke(current,"copy")({value});else self.set(key,value);
            }
        });return {};
    }
    if(name=="toJSON")return material_toJSON(self,arg(args,0),imports);
    if(name=="fromJSON")return material_fromJSON(self,arg(args,0),arg(args,1),imports);
    if(name=="clone"){const auto clone=self.get("constructor").construct();return invoke(clone,"copy")({self});}
    if(name=="copy")return copy(self,arg(args,0),imports);
    if(name=="dispose"){const auto dispatch=invoke(self,"dispatchEvent");auto event=JSValue::object();own(event,"type","dispose");dispatch({event});return {};}
    throw std::logic_error("Unknown Material method");
}
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeMaterialConstructor(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Material");
    auto imports=JSValue::object(nullptr);own(imports,"Color",nativeColorConstructor());own(imports,"Vector2",nativeVector2Constructor());own(imports,"Plane",nativePlaneConstructor());
    own(imports,"Array",JSValue::intrinsicArrayConstructor());own(imports,"Object",JSValue::intrinsicObjectConstructor());
    const auto base=EventDispatcher::runtimeConstructor();auto holder=JSValue::object(nullptr),state=JSValue::object(nullptr);own(state,"id",0);own(holder,"imports",imports);own(holder,"state",state);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args&,const Args& captures)->JSValue{
        const auto holder=captures[0];auto self=holder.get("constructor").getPrototype().construct({},target);
        return material_constructor(self,holder.get("imports"),holder.get("state"));
    },{holder},"Material",0);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MaterialSource::Material);
    for(const auto& [name,arity,text]:std::initializer_list<std::tuple<const char*,unsigned,const char*>>{
        {"get_alphaTest",0,MaterialSource::get_alphaTest},{"set_alphaTest",1,MaterialSource::set_alphaTest},
        {"onBeforeRender",0,MaterialSource::onBeforeRender},{"onBeforeCompile",0,MaterialSource::onBeforeCompile},{"customProgramCacheKey",0,MaterialSource::customProgramCacheKey},
        {"setValues",1,MaterialSource::setValues},{"toJSON",1,MaterialSource::toJSON},{"fromJSON",2,MaterialSource::fromJSON},{"clone",0,MaterialSource::clone},{"copy",1,MaterialSource::copy},{"dispose",0,MaterialSource::dispose},{"set_needsUpdate",1,MaterialSource::set_needsUpdate}}){
        const std::string key=name;const bool getter=key.starts_with("get_"),setter=key.starts_with("set_");
        auto function=JSValue::function(method,{name,imports},getter?"get "+key.substr(4):setter?"set "+key.substr(4):key,arity);function.setFunctionSourceText(text);
        const auto property=getter||setter?key.substr(4):key;auto d=prototype.getOwnPropertyDescriptor(property).value_or(JSPropertyDescriptor{});d.configurable=true;
        if(getter)d.getter=function;else if(setter)d.setter=function;else{d.value=function;d.writable=true;}prototype.defineProperty(property,d);
    }
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeMaterialModule(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    auto bindings=JSValue::object(nullptr);own(bindings,"Material",nativeMaterialConstructor());auto module=JSValue::object();own(module,"Material",bindings.get("Material"));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMaterialBinding(const std::string& name){const auto keeper=nativeMaterialModule();return lexical().lock()->get(name);}

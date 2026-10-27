// Port of three.js CubeTextureNode.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/ReflectVector.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "textures/CubeTexture.h"
#include "constants.h"
#include "utils.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
#include "nodes/accessors/CubeTextureNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& c){
    const auto name=c[0],imports=c[1];
    if(name.strictEquals("setUpdateMatrix"))return {};
    if(name.strictEquals("getInputType"))return self.get("value").get("isDepthTexture").strictEquals(true)?JSValue("cubeDepthTexture"):JSValue("cubeTexture");
    if(name.strictEquals("getDefaultUV")){
        const auto texture=self.get("value");
        if(texture.get("mapping").strictEquals(imports.get("CubeReflectionMapping")))return imports.get("reflectVector");
        if(texture.get("mapping").strictEquals(imports.get("CubeRefractionMapping")))return imports.get("refractVector");
        ThreeUtils::error({"CubeTextureNode: Mapping \"%s\" not supported.",texture.get("mapping")});return imports.get("vec3").call({}, {0,0,0});
    }
    const auto builder=arg(args,0);auto uv=arg(args,1);
    if(name.strictEquals("setupUV")){
        const auto texture=self.get("value");
        if(builder.get("renderer").get("coordinateSystem").strictEquals(imports.get("WebGPUCoordinateSystem"))||(!texture.get("isRenderTargetTexture").toBoolean()&&!texture.get("isDepthTexture").toBoolean())){
            const auto fn=imports.get("vec3"),x=invoke(uv.get("x"),"negate")();const auto yz=uv.get("yz");uv=fn.call({}, {x,yz});
        }return uv;
    }
    if(name.strictEquals("generateUV")){const auto build=invoke(uv,"build");return build({builder,self.get("sampler").strictEquals(true)?JSValue("vec3"):JSValue("ivec3")});}
    throw std::logic_error("Unknown CubeTextureNode method");
}
JSValue factory(const JSValue&,const Args& args,const Args& c){
    const auto value=arg(args,0,c[1]),kind=c[0];
    if(kind.strictEquals("uniformCubeTexture")){auto node=c[2].call({}, {value});node.set("mergeable",false);return node;}
    const auto uv=arg(args,1,nullptr),level=arg(args,2,nullptr),bias=arg(args,3,nullptr);JSValue node;
    if(value.toBoolean()&&value.get("isCubeTextureNode").strictEquals(true)){
        node=invoke(value,"clone")();node.set("referenceNode",value);
        if(!uv.isNull())node.set("uvNode",c[3].call({}, {uv}));
        if(!level.isNull())node.set("levelNode",c[3].call({}, {level}));
        if(!bias.isNull())node.set("biasNode",c[3].call({}, {bias}));
    }else node=c[2].call({}, {value,uv,level,bias});return node;
}
}
JSValue nativeCubeTextureNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"CubeTextureNode module");
    const auto base=nativeTextureNodeConstructor(),reflect=nativeReflectVectorModule(),tsl=nativeTSLBase();
    const auto constants=nativeConstantsModule(),cube=nativeCubeTextureConstructor();auto imports=JSValue::object(nullptr);
    for(const auto* name:{"reflectVector","refractVector"})own(imports,name,nativeReflectVectorBinding(name));
    for(const auto* name:{"nodeObject","nodeProxy","vec3"})own(imports,name,nativeTSLBaseBinding(name));
    for(const auto* name:{"CubeReflectionMapping","CubeRefractionMapping","WebGPUCoordinateSystem"})own(imports,name,nativeConstantsBinding(name));
    const auto empty=cube.construct();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{arg(args,0),arg(args,1,nullptr),arg(args,2,nullptr),arg(args,3,nullptr)});self.set("isCubeTextureNode",true);return self;
    },{holder},"CubeTextureNode",1);holder.set("constructor",ctor);ctor.setPrototype(base);
    ctor.setFunctionSourceText(CubeTextureNodeSource::classSource);
    auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));JSPropertyDescriptor type;type.configurable=true;auto getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "CubeTextureNode";},{},"get type",0);getter.setFunctionSourceText(CubeTextureNodeSource::typeSource);type.getter=getter;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getInputType",0},{"getDefaultUV",0},{"setUpdateMatrix",0},{"setupUV",2},{"generateUV",2}}){JSPropertyDescriptor d;auto fn=JSValue::function(method,{name,imports},name,arity);const std::string key=name;fn.setFunctionSourceText(key=="getInputType"?CubeTextureNodeSource::getInputTypeSource:key=="getDefaultUV"?CubeTextureNodeSource::getDefaultUVSource:key=="setUpdateMatrix"?CubeTextureNodeSource::setUpdateMatrixSource:key=="setupUV"?CubeTextureNodeSource::setupUVSource:CubeTextureNodeSource::generateUVSource);d.value=fn;d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    const auto proxy=imports.get("nodeProxy").call({}, {ctor});const auto limited=invoke(proxy,"setParameterLength")({1,4});auto factoryBase=invoke(limited,"setName")({"cubeTexture"});factoryBase.setFunctionSourceText(CubeTextureNodeSource::cubeTextureBaseSource);own(bindings,"cubeTextureBase",factoryBase);
    for(const auto* name:{"cubeTexture","uniformCubeTexture"}){auto fn=JSValue::function(factory,{name,empty,factoryBase,imports.get("nodeObject")},name,0);fn.setFunctionSourceText(std::string(name)=="cubeTexture"?CubeTextureNodeSource::cubeTextureSource:CubeTextureNodeSource::uniformCubeTextureSource);own(bindings,name,fn);}
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeCubeTextureNodeBinding(const std::string& name){const auto keeper=nativeCubeTextureNodeModule();return lexical().lock()->get(name);}
JSValue nativeCubeTextureNodeConstructor(){return nativeCubeTextureNodeBinding("default");}
CubeTextureNode::CubeTextureNode(const JSValue& value,const JSValue& uv,const JSValue& level,const JSValue& bias):object_(constructor().construct({value,uv,level,bias})){}
JSValue CubeTextureNode::getInputType()const{return object_.get("getInputType").call(object_);}
JSValue CubeTextureNode::getDefaultUV()const{return object_.get("getDefaultUV").call(object_);}
void CubeTextureNode::setUpdateMatrix(const JSValue& value)const{object_.get("setUpdateMatrix").call(object_,{value});}
JSValue CubeTextureNode::setupUV(const JSValue& builder,const JSValue& uv)const{return object_.get("setupUV").call(object_,{builder,uv});}
JSValue CubeTextureNode::generateUV(const JSValue& builder,const JSValue& uv)const{return object_.get("generateUV").call(object_,{builder,uv});}

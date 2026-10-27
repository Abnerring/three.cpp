// Native managed port of pinned three.js MaterialNode.js.
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/accessors/MaterialReferenceNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/core/UniformNode.h"
#include "nodes/display/NormalMapNode.h"
#include "nodes/display/BumpMapNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSMap.h"
#include "core/JSGlobal.h"
#include "math/Vector2.h"
#include "constants.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
#include "MaterialNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
// Imported format constants are numbers. Abstract equality converts the other
// operand with the default hint; null/undefined and symbols never equal a number.
bool formatEquals(JSValue value,const JSValue& number){
    if(value.isObject())value=value.toPrimitive();
    if(value.isNull()||value.isUndefined()||value.isSymbol())return false;
    return value.toNumber()==number.toNumber();
}
JSValue setup(const JSValue& self,const JSValue& builder,const JSValue& imports,const JSValue& bindings){
    const auto material=builder.get("context").get("material"),scope=self.get("scope"),ctor=bindings.get("default");auto node=JSValue(nullptr);
    const auto is=[&](const char* key){return scope.strictEquals(ctor.get(key));};
    const auto valid=[&](const char* key){return material.get(key).toBoolean()&&material.get(key).get("isTexture").strictEquals(true);};
    if(is("COLOR")){
        const auto color=!material.get("color").isUndefined()?invoke(self,"getColor")({scope}):imports.get("vec3").call();
        if(valid("map")){const auto mul=invoke(color,"mul");node=mul({invoke(self,"getTexture")({"map"})});}else node=color;
    }else if(is("OPACITY")){
        const auto opacity=invoke(self,"getFloat")({scope});if(valid("alphaMap")){const auto mul=invoke(opacity,"mul");node=mul({invoke(self,"getTexture")({"alpha"})});}else node=opacity;
    }else if(is("SPECULAR_STRENGTH")){
        node=valid("specularMap")?invoke(self,"getTexture")({"specular"}).get("r"):imports.get("float").call({}, {1});
    }else if(is("SPECULAR_INTENSITY")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("specularIntensityMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("a")});}else node=base;
    }else if(is("SPECULAR_COLOR")){
        const auto base=invoke(self,"getColor")({scope});if(valid("specularColorMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("rgb")});}else node=base;
    }else if(is("ROUGHNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("roughnessMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("g")});}else node=base;
    }else if(is("DIFFUSE_ROUGHNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("diffuseRoughnessMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("r")});}else node=base;
    }else if(is("METALNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("metalnessMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("b")});}else node=base;
    }else if(is("EMISSIVE")){
        const auto intensity=invoke(self,"getFloat")({"emissiveIntensity"}),base=invoke(invoke(self,"getColor")({scope}),"mul")({intensity});
        if(valid("emissiveMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope})});}else node=base;
    }else if(is("NORMAL")){
        if(material.get("normalMap").toBoolean()){
            const auto fn=imports.get("normalMap"),texture=invoke(self,"getTexture")({"normal"}),scale=invoke(self,"getCache")({"normalScale","vec2"});node=fn.call({}, {texture,scale});
            node.set("normalMapType",material.get("normalMapType"));
            if(formatEquals(material.get("normalMap").get("format"),imports.get("RGFormat"))||formatEquals(material.get("normalMap").get("format"),imports.get("RED_GREEN_RGTC2_Format"))||formatEquals(material.get("normalMap").get("format"),imports.get("RG11_EAC_Format")))node.set("unpackNormalMode",imports.get("NormalRGPacking"));
        }else if(material.get("bumpMap").toBoolean()){
            const auto fn=imports.get("bumpMap"),texture=invoke(self,"getTexture")({"bump"}).get("r"),scale=invoke(self,"getFloat")({"bumpScale"});node=fn.call({}, {texture,scale});
        }else node=imports.get("normalView");
    }else if(is("CLEARCOAT")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("clearcoatMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("r")});}else node=base;
    }else if(is("CLEARCOAT_ROUGHNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("clearcoatRoughnessMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("g")});}else node=base;
    }else if(is("CLEARCOAT_NORMAL")){
        if(material.get("clearcoatNormalMap").toBoolean()){
            const auto fn=imports.get("normalMap"),texture=invoke(self,"getTexture")({scope}),scale=invoke(self,"getCache")({add(scope,"Scale"),"vec2"});node=fn.call({}, {texture,scale});
        }else node=imports.get("normalView");
    }else if(is("SHEEN")){
        const auto mul=invoke(invoke(self,"getColor")({"sheenColor"}),"mul");const auto base=mul({invoke(self,"getFloat")({"sheen"})});
        if(valid("sheenColorMap")){const auto multiply=invoke(base,"mul");node=multiply({invoke(self,"getTexture")({"sheenColor"}).get("rgb")});}else node=base;
    }else if(is("SHEEN_ROUGHNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(valid("sheenRoughnessMap")){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("a")});}else node=base;node=invoke(node,"clamp")({.0001,1.0});
    }else if(is("ANISOTROPY")){
        if(valid("anisotropyMap")){
            const auto polar=invoke(self,"getTexture")({scope}),mat2=imports.get("mat2"),vector=bindings.get("materialAnisotropyVector");
            const auto x=vector.get("x"),y=vector.get("y"),negative=invoke(vector.get("y"),"negate")(),last=vector.get("x");const auto matrix=mat2.call({}, {x,y,negative,last});
            const auto multiply=invoke(matrix,"mul");const auto rg=invoke(polar.get("rg"),"mul")({2.0});const auto subtract=invoke(rg,"sub");const auto shifted=subtract({imports.get("vec2").call({}, {1.0})});
            const auto normalized=invoke(shifted,"normalize")();const auto times=invoke(normalized,"mul");node=multiply({times({polar.get("b")})});
        }else node=bindings.get("materialAnisotropyVector");
    }else if(is("IRIDESCENCE_THICKNESS")){
        const auto reference=imports.get("reference"),maximum=reference.call({}, {"1","float",material.get("iridescenceThicknessRange")});
        if(material.get("iridescenceThicknessMap").toBoolean()){
            const auto minimum=reference.call({}, {"0","float",material.get("iridescenceThicknessRange")});
            const auto multiply=invoke(invoke(maximum,"sub")({minimum}),"mul");const auto value=multiply({invoke(self,"getTexture")({scope}).get("g")});node=invoke(value,"add")({minimum});
        }else node=maximum;
    }else if(is("TRANSMISSION")){
        const auto base=invoke(self,"getFloat")({scope});if(material.get("transmissionMap").toBoolean()){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("r")});}else node=base;
    }else if(is("THICKNESS")){
        const auto base=invoke(self,"getFloat")({scope});if(material.get("thicknessMap").toBoolean()){const auto mul=invoke(base,"mul");node=mul({invoke(self,"getTexture")({scope}).get("g")});}else node=base;
    }else if(is("IOR"))node=invoke(self,"getFloat")({scope});
    else if(is("LIGHT_MAP")){
        if(material.get("lightMap").toBoolean()){const auto mul=invoke(invoke(self,"getTexture")({scope}).get("rgb"),"mul");node=mul({invoke(self,"getFloat")({"lightMapIntensity"})});}else node=imports.get("vec3").call({}, {0.0});
    }else if(is("AO")){
        if(material.get("aoMap").toBoolean()){const auto mul=invoke(invoke(invoke(self,"getTexture")({scope}).get("r"),"sub")({1.0}),"mul");node=invoke(mul({invoke(self,"getFloat")({"aoMapIntensity"})}),"add")({1.0});}else node=imports.get("float").call({}, {1.0});
    }else if(is("LINE_DASH_OFFSET"))node=material.get("dashOffset").toBoolean()?invoke(self,"getFloat")({scope}):imports.get("float").call({}, {0});
    else {const auto type=invoke(self,"getNodeType")({builder});node=invoke(self,"getCache")({scope,type});}
    return node;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    const auto self=receiver,name=captures[0],imports=captures[1],bindings=captures[2];
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("getCache")){
        const auto property=arg(args,0),type=arg(args,1),cache=imports.get("_propertyCache");auto node=invoke(cache,"get")({property});
        if(node.isUndefined()){node=imports.get("materialReference").call({}, {property,type});invoke(cache,"set")({property,node});}return node;
    }
    if(name.strictEquals("getFloat"))return invoke(self,"getCache")({arg(args,0),"float"});
    if(name.strictEquals("getColor"))return invoke(self,"getCache")({arg(args,0),"color"});
    if(name.strictEquals("getTexture")){const auto get=invoke(self,"getCache");const auto property=arg(args,0);return get({property.strictEquals("map")?property:add(property,"Map"),"texture"});}
    if(name.strictEquals("setup"))return setup(self,arg(args,0),imports,bindings);
    throw std::logic_error("Unknown MaterialNode method");
}
JSValue anisotropyReference(const JSValue&,const Args& args,const Args&){return arg(args,0).get("material");}
JSValue anisotropyRender(const JSValue& self,const Args& args,const Args&){
    const auto material=arg(args,0).get("material");const auto set=invoke(self.get("value"),"set");
    const auto left=material.get("anisotropy");const auto cosine=invoke(nativeGlobalBinding("Math"),"cos");const auto rotation=material.get("anisotropyRotation");const auto cosValue=cosine({rotation});const auto leftNumber=left.toNumber(),cosNumber=cosValue.toNumber();const auto x=leftNumber*cosNumber;
    const auto right=material.get("anisotropy");const auto sine=invoke(nativeGlobalBinding("Math"),"sin");const auto angle=material.get("anisotropyRotation");const auto sinValue=sine({angle});const auto rightNumber=right.toNumber(),sinNumber=sinValue.toNumber();const auto y=rightNumber*sinNumber;set({x,y});return {};
}
}
JSValue nativeMaterialNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialNode module");
    const auto node=nativeNodeConstructor(),reference=nativeReferenceNodeModule(),materialReference=nativeMaterialReferenceNodeModule(),normal=nativeNormalModule(),tsl=nativeTSLBase(),uniform=nativeUniformNodeConstructor(),uniformFactory=nativeUniformFactory(),normalMap=nativeNormalMapNodeModule(),bumpMap=nativeBumpMapNodeModule(),vector=nativeVector2Constructor(),constants=nativeConstantsModule();
    auto imports=JSValue::object(nullptr),bindings=JSValue::object(nullptr),holder=JSValue::object(nullptr);own(imports,"reference",nativeReferenceNodeBinding("reference"));own(imports,"materialReference",nativeMaterialReferenceNodeBinding("materialReference"));own(imports,"normalView",nativeNormalBinding("normalView"));
    for(const auto* name:{"nodeImmutable","float","vec2","vec3","mat2"})own(imports,name,nativeTSLBaseBinding(name));own(imports,"uniform",uniformFactory);own(imports,"normalMap",nativeNormalMapNodeBinding("normalMap"));own(imports,"bumpMap",nativeBumpMapNodeBinding("bumpMap"));
    for(const auto* name:{"RGFormat","RED_GREEN_RGTC2_Format","RG11_EAC_Format","NormalRGPacking"})own(imports,name,nativeConstantsBinding(name));own(imports,"_propertyCache",nativeGlobalBinding("Map").construct());
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{auto self=captures[0].get("constructor").getPrototype().construct({},target);self.set("scope",arg(args,0));return self;},{holder},"MaterialNode",1);
    holder.set("constructor",ctor);ctor.setPrototype(node);auto p=ctor.get("prototype");p.setPrototype(node.get("prototype"));ctor.setFunctionSourceText(MaterialNodeSource::MaterialNode);
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "MaterialNode";},{},"get type",0);type.getter->setFunctionSourceText(MaterialNodeSource::get_type);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"getCache",2},{"getFloat",1},{"getColor",1},{"getTexture",1},{"setup",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,imports,bindings},name,arity);d.value->setFunctionSourceText(MaterialNodeSource::get(name));d.writable=d.configurable=true;p.defineProperty(name,d);}
    for(const auto& [key,value]:MaterialNodeSource::constants)own(ctor,key,value);own(bindings,"default",ctor);
    for(const auto& [key,scopeName]:MaterialNodeSource::immutables)own(bindings,key,imports.get("nodeImmutable").call({}, {ctor,ctor.get(scopeName)}));
    const auto u=imports.get("uniform").call({}, {vector.construct()});const auto onReference=invoke(u,"onReference");auto referenceCallback=JSValue::constructor(anisotropyReference,{},"",1,JSValue::ConstructorKind::Callable);referenceCallback.setFunctionSourceText(MaterialNodeSource::anisotropyReference);
    const auto onRender=invoke(onReference({referenceCallback}),"onRenderUpdate");auto renderCallback=JSValue::constructor(anisotropyRender,{},"",1,JSValue::ConstructorKind::Callable);renderCallback.setFunctionSourceText(MaterialNodeSource::anisotropyRender);own(bindings,"materialAnisotropyVector",onRender({renderCallback}));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMaterialNodeBinding(const std::string& name){const auto keeper=nativeMaterialNodeModule();return lexical().lock()->get(name);}
JSValue nativeMaterialNodeConstructor(){return nativeMaterialNodeBinding("default");}

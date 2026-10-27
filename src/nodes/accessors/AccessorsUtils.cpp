
#include "nodes/accessors/AccessorsUtils.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Tangent.h"
#include "nodes/accessors/Bitangent.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
#include <algorithm>
namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue bentBody(const JSValue&,const Args&,const Args& c){
 const auto table=c[0],axis=table.get("anisotropyB");auto bent=invoke(axis,"cross")({table.get("positionViewDirection")});bent=invoke(invoke(bent,"cross")({axis}),"normalize")();
 const auto mix=table.get("mix"),normal=table.get("normalView");const auto multiply=invoke(table.get("anisotropy"),"mul");
 const auto factor=invoke(invoke(invoke(multiply({invoke(table.get("roughness"),"oneMinus")()}),"oneMinus")(),"pow2")(),"pow2")();
 return invoke(mix.call({}, {bent,normal,factor}),"normalize")();
}
}
JSValue nativeAccessorsUtilsModule(){
 using namespace TSLFunctionRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"AccessorsUtils module");
 const auto normal=nativeNormalModule(),tangent=nativeTangentModule(),bitangent=nativeBitangentModule(),tsl=nativeTSLBase(),math=nativeMathNodeModule(),properties=nativePropertyNodeModule(),position=nativePositionModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 own(table,"normalView",nativeNormalBinding("normalView"));own(table,"mix",nativeMathNodeBinding("mix"));
 for(const auto* name:{"anisotropy","anisotropyB","roughness"})own(table,name,nativePropertyNodeBinding(name));own(table,"positionViewDirection",nativePositionBinding("positionViewDirection"));
 own(bindings,"TBNViewMatrix",invoke(nativeTSLBaseBinding("mat3").call({}, {nativeTangentBinding("tangentView"),nativeBitangentBinding("bitangentView"),nativeNormalBinding("normalView")}),"toVar")({"TBNViewMatrix"}));
 const auto direction=invoke(table.get("positionViewDirection"),"mul")({bindings.get("TBNViewMatrix")});own(bindings,"parallaxDirection",direction);
 auto parallax=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{return invoke(arg(a,0),"sub")({invoke(c[0],"mul")({arg(a,1)})});},{direction},"parallaxUV",2);parallax.setFunctionSourceText(AccessorsUtilsSource::parallaxUVSource);own(bindings,"parallaxUV",parallax);
 auto callback=JSValue::function(bentBody,{table},"",0);callback.setFunctionSourceText(AccessorsUtilsSource::bentNormalViewCallbackSource);own(bindings,"bentNormalView",invoke(nativeTSLBaseBinding("Fn").call({}, {callback}),"once")().call());
 auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeAccessorsUtilsBinding(const std::string& name){const auto keeper=nativeAccessorsUtilsModule();return lexical().lock()->get(name);}

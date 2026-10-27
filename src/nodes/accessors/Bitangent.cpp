
#include "nodes/accessors/Bitangent.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Tangent.h"
#include "nodes/accessors/TangentUtils.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue getBitangentBody(const JSValue&,const Args& args,const Args& c){
 auto iterator=getJSIterator(arg(args,0));const auto a=stepJSIterator(iterator),b=iterator.done?std::optional<JSValue>{}:stepJSIterator(iterator);
 const auto cross=a?*a:JSValue{},name=b?*b:JSValue{};if(!iterator.done)closeJSIterator(iterator);
 const auto builder=arg(args,1);auto value=invoke(cross,"mul")({c[0].get("tangentGeometry").get("w")}).get("xyz");
 if(builder.get("subBuildFn").strictEquals("NORMAL")&&!invoke(builder,"isFlatShading")().strictEquals(true))value=invoke(value,"toVarying")({name});return value;
}
JSValue viewBody(const JSValue&,const Args& args,const Args& c){
 const auto builder=arg(args,0),table=c[0];auto node=JSValue{};
 if(builder.get("subBuildFn").strictEquals("VERTEX")||invoke(builder.get("geometry"),"hasAttribute")({"tangent"}).toBoolean()){
  const auto getBitangent=table.get("getBitangent"),cross=invoke(table.get("normalView"),"cross")({table.get("tangentView")});node=invoke(getBitangent.call({}, {cross,"v_bitangentView"}),"normalize")();
 }else node=table.get("bitangentViewFrame");
 if(!invoke(builder,"isFlatShading")().strictEquals(true))node=table.get("negateOnBackSide").call({}, {node});return node;
}
}
JSValue nativeBitangentModule(){
 using namespace TSLFunctionRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Bitangent module");
 const auto tsl=nativeTSLCore(),normal=nativeNormalModule(),tangent=nativeTangentModule(),utils=nativeTangentUtilsModule(),facing=nativeFrontFacingNodeModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 for(const auto* name:{"tangentGeometry","tangentLocal","tangentView","tangentWorld"})own(table,name,nativeTangentBinding(name));
 own(table,"normalView",nativeNormalBinding("normalView"));own(table,"bitangentViewFrame",nativeTangentUtilsBinding("bitangentViewFrame"));own(table,"negateOnBackSide",nativeFrontFacingNodeBinding("negateOnBackSide"));
 auto callback=JSValue::function(getBitangentBody,{table},"",2);callback.setFunctionSourceText(BitangentSource::getBitangentCallbackSource);
 auto scopes=JSValue::array();own(scopes,"0","NORMAL");own(table,"getBitangent",invoke(nativeTSLCoreBinding("Fn").call({}, {callback}),"once")({scopes}));
 for(const auto* space:{"Geometry","Local"}){
  const std::string normalName=std::string("normal")+space,tangentName=std::string("tangent")+space,name=std::string("bitangent")+space,varying=std::string("v_")+name;
  const auto fn=table.get("getBitangent"),cross=invoke(nativeNormalBinding(normalName),"cross")({table.get(tangentName)});
  own(bindings,name.c_str(),invoke(invoke(fn.call({}, {cross,varying}),"normalize")(),"toVar")({name}));
 }
 auto viewCallback=JSValue::function(viewBody,{table},"",1);viewCallback.setFunctionSourceText(BitangentSource::bitangentViewCallbackSource);
 auto viewScopes=JSValue::array();own(viewScopes,"0","NORMAL");own(viewScopes,"1","VERTEX");
 const auto viewFn=invoke(nativeTSLCoreBinding("Fn").call({}, {viewCallback,"vec3"}),"once")({viewScopes});own(bindings,"bitangentView",invoke(viewFn.call(),"toVar")({"bitangentView"}));
 const auto getWorld=table.get("getBitangent"),worldCross=invoke(nativeNormalBinding("normalWorld"),"cross")({table.get("tangentWorld")});
 own(bindings,"bitangentWorld",invoke(invoke(getWorld.call({}, {worldCross,"v_bitangentWorld"}),"normalize")(),"toVar")({"bitangentWorld"}));
 auto module=JSValue::object();for(const auto* name:{"bitangentGeometry","bitangentLocal","bitangentView","bitangentWorld"})own(module,name,bindings.get(name));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBitangentBinding(const std::string& name){const auto keeper=nativeBitangentModule();return lexical().lock()->get(name);}

#include "nodes/display/NormalMapNode.h"

NormalMap::Channels NormalMap::channelsFor(Packing packing) {
    // A green alpha map keeps its pair in the second and fourth parts rather than the first two.
    if (packing == Packing::RedGreen) return Channels::Xy;
    if (packing == Packing::GreenAlpha) return Channels::Yw;
    return Channels::Whole;
}

bool NormalMap::isUnexpected(Packing packing) {
    // The two packed modes and the unpacked one are all handled; nothing else is. Since the
    // enumeration has no other member, this is false for every value it can hold, which is
    // worth stating rather than leaving the caller to work out.
    return false;
}

std::string NormalMap::unexpectedModeMessage(const std::string& modeName) {
    return "THREE.NodeMaterial: Unexpected unpack normal mode: " + modeName;
}

double NormalMap::signedValue(double sampled) {
    // Twice the value less one.
    return sampled * 2.0 - 1.0;
}

#include "nodes/core/Node.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/AccessorsUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/Packing.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
#include "constants.h"
#include "utils.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue setup(const JSValue& self,const Args& args,const Args& c){
 const auto table=c[2],builder=arg(args,0),type=self.get("normalMapType"),scaleNode=self.get("scaleNode"),mode=self.get("unpackNormalMode");
 auto normal=invoke(invoke(self.get("node"),"mul")({2}),"sub")({1});
 if(type.strictEquals(table.get("TangentSpaceNormalMap"))){
  if(mode.strictEquals(table.get("NormalRGPacking")))normal=table.get("unpackNormal").call({}, {normal.get("xy")});
  else if(mode.strictEquals(table.get("NormalGAPacking")))normal=table.get("unpackNormal").call({}, {normal.get("yw")});
  else if(!mode.strictEquals(table.get("NoNormalPacking")))ThreeUtils::error({"THREE.NodeMaterial: Unexpected unpack normal mode: "+mode.toString()});
 }else if(!mode.strictEquals(table.get("NoNormalPacking"))){
  const auto typeText=type.toString();ThreeUtils::error({"THREE.NodeMaterial: Normal map type '"+typeText+"' is not compatible with unpack normal mode '"+mode.toString()+"'"});
 }
 if(!scaleNode.isNull()){
  auto scale=scaleNode;if(invoke(builder,"isFlatShading")().strictEquals(true))scale=table.get("negateOnBackSide").call({}, {scale});
  const auto vec3=table.get("vec3"),xy=invoke(normal.get("xy"),"mul")({scale}),z=normal.get("z");normal=vec3.call({}, {xy,z});
 }
 if(type.strictEquals(table.get("ObjectSpaceNormalMap")))return table.get("transformNormalToView").call({}, {normal});
 if(type.strictEquals(table.get("TangentSpaceNormalMap")))return invoke(invoke(table.get("TBNViewMatrix"),"mul")({normal}),"normalize")();
 ThreeUtils::error({"NodeMaterial: Unsupported normal map type: "+type.toString()});return table.get("normalView");
}
}
JSValue nativeNormalMapNodeModule(){
 using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"NormalMapNode module");
 const auto node=nativeNodeConstructor(),normal=nativeNormalModule(),accessors=nativeAccessorsUtilsModule(),tsl=nativeTSLBase(),constants=nativeConstantsModule(),facing=nativeFrontFacingNodeModule(),packing=nativePackingModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 for(const auto* name:{"TangentSpaceNormalMap","ObjectSpaceNormalMap","NoNormalPacking","NormalRGPacking","NormalGAPacking"})own(table,name,nativeConstantsBinding(name));
 for(const auto* name:{"normalView","transformNormalToView"})own(table,name,nativeNormalBinding(name));own(table,"TBNViewMatrix",nativeAccessorsUtilsBinding("TBNViewMatrix"));own(table,"vec3",nativeTSLBaseBinding("vec3"));own(table,"negateOnBackSide",nativeFrontFacingNodeBinding("negateOnBackSide"));own(table,"unpackNormal",nativePackingBinding("unpackNormal"));
 auto ctor=makeConstructor("NormalMapNode",1,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
  const auto scale=arg(a,1,nullptr);auto self=superConstruct(target,c,{"vec3"});self.set("node",arg(a,0));self.set("scaleNode",scale);self.set("normalMapType",nativeConstantsBinding("TangentSpaceNormalMap"));self.set("unpackNormalMode",nativeConstantsBinding("NoNormalPacking"));return self;
 },{{"setup",1}},setup,{table});
 ctor.setFunctionSourceText(NormalMapNodeSource::classSource);auto prototype=ctor.get("prototype");auto method=prototype.get("setup");method.setFunctionSourceText(NormalMapNodeSource::setupSource);auto getter=ctor.getOwnPropertyDescriptor("type")->getter.value();getter.setFunctionSourceText(NormalMapNodeSource::typeSource);
 own(bindings,"default",ctor);own(bindings,"normalMap",invoke(nativeTSLBaseBinding("nodeProxy").call({}, {ctor}),"setParameterLength")({1,2}));
 auto module=JSValue::object();own(module,"default",ctor);own(module,"normalMap",bindings.get("normalMap"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeNormalMapNodeConstructor(){return nativeNormalMapNodeBinding("default");}
JSValue nativeNormalMapNodeBinding(const std::string& name){const auto keeper=nativeNormalMapNodeModule();return lexical().lock()->get(name);}

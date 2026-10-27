#include "nodes/display/BumpMapNode.h"

bool BumpMap::skipsBump(bool wireframeIsTrue) {
    // Only a material that says exactly true is skipped; one that says nothing is not.
    return wireframeIsTrue;
}

double BumpMap::bumpScale(bool hasScale, double scale) {
    // A scale that is present is used however small, because the test is against null and not
    // for truth.
    return hasScale ? scale : 1.0;
}

double BumpMap::determinant(double sigmaXDotR1) {
    // The determinant itself; the caller multiplies in the facing.
    return sigmaXDotR1;
}

double BumpMap::gradientSign(double determinantValue) {
    // The sign only: minus one, nought or one.
    if (determinantValue > 0.0) return 1.0;
    if (determinantValue < 0.0) return -1.0;
    return 0.0;
}

#include "nodes/core/Node.h"
#include "nodes/accessors/UV.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue uvSample(const JSValue&,const Args& a,const Args& c){
 const auto node=arg(a,0);if(c[0].strictEquals("identity"))return node;
 const auto add=invoke(node,"add");return add({invoke(node,c[0].strictEquals("dx")?"dFdx":"dFdy")()});
}
JSValue getUV(const JSValue&,const Args& a,const Args& c){const auto callback=c[0];auto uv=arg(a,0).get("uvNode");if(!uv.toBoolean())uv=c[1].call();return callback.call({}, {uv});}
JSValue sampleTexture(const JSValue&,const Args& a,const Args& c){
 const auto callContext=invoke(invoke(c[0],"isolate")(),"context");auto options=JSValue::object();auto get=JSValue::function(getUV,{arg(a,0),c[1]},"getUV",1);get.setFunctionSourceText(BumpMapNodeSource::getUVSource);own(options,"getUV",get);own(options,"forceUVContext",true);return callContext({options});
}
JSValue dHdxyBody(const JSValue&,const Args& a,const Args& c){
 const auto inputs=arg(a,0),textureNode=inputs.get("textureNode"),bumpScale=inputs.get("bumpScale"),table=c[0];
 auto sample=JSValue::function(sampleTexture,{textureNode,table.get("uv")},"sampleTexture",1);sample.setFunctionSourceText(BumpMapNodeSource::sampleTextureSource);
 auto identity=JSValue::function(uvSample,{"identity"},"",1);identity.setFunctionSourceText(BumpMapNodeSource::uvIdentitySource);
 const auto floatFn=table.get("float"),Hll=floatFn.call({}, {sample.call({}, {identity})}),vec2=table.get("vec2"),firstFloat=table.get("float");
 auto dx=JSValue::function(uvSample,{"dx"},"",1);dx.setFunctionSourceText(BumpMapNodeSource::uvDxSource);const auto x=invoke(firstFloat.call({}, {sample.call({}, {dx})}),"sub")({Hll});
 const auto secondFloat=table.get("float");auto dy=JSValue::function(uvSample,{"dy"},"",1);dy.setFunctionSourceText(BumpMapNodeSource::uvDySource);const auto y=invoke(secondFloat.call({}, {sample.call({}, {dy})}),"sub")({Hll});
 return invoke(vec2.call({}, {x,y}),"mul")({bumpScale});
}
JSValue perturbBody(const JSValue&,const Args& a,const Args& c){
 const auto inputs=arg(a,0),pos=inputs.get("surf_pos"),normal=inputs.get("surf_norm"),dHdxy=inputs.get("dHdxy");
 const auto x=invoke(invoke(pos,"dFdx")(),"normalize")(),y=invoke(invoke(pos,"dFdy")(),"normalize")();
 const auto R1=invoke(y,"cross")({normal}),R2=invoke(normal,"cross")({x}),det=invoke(invoke(x,"dot")({R1}),"mul")({c[0]});
 const auto multiply=invoke(invoke(det,"sign")(),"mul");const auto grad=multiply({invoke(invoke(dHdxy.get("x"),"mul")({R1}),"add")({invoke(dHdxy.get("y"),"mul")({R2})})});
 return invoke(invoke(invoke(invoke(det,"abs")(),"mul")({normal}),"sub")({grad}),"normalize")();
}
JSValue setup(const JSValue& self,const Args& a,const Args& c){
 const auto table=c[2];if(arg(a,0).get("material").get("wireframe").strictEquals(true))return table.get("normalView");
 const auto scale=!self.get("scaleNode").isNull()?self.get("scaleNode"):JSValue(1),forward=table.get("dHdxy_fwd");auto inputs=JSValue::object();own(inputs,"textureNode",self.get("textureNode"));own(inputs,"bumpScale",scale);const auto derivatives=forward.call({}, {inputs});
 const auto perturb=table.get("perturbNormalArb");auto p=JSValue::object();own(p,"surf_pos",table.get("positionView"));own(p,"surf_norm",table.get("normalView"));own(p,"dHdxy",derivatives);return perturb.call({}, {p});
}
}
JSValue nativeBumpMapNodeModule(){
 using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BumpMapNode module");
 const auto node=nativeNodeConstructor(),uv=nativeUVModule(),normal=nativeNormalModule(),position=nativePositionModule(),facing=nativeFrontFacingNodeModule(),tsl=nativeTSLBase();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);own(table,"uv",nativeUVBinding("uv"));own(table,"normalView",nativeNormalBinding("normalView"));own(table,"positionView",nativePositionBinding("positionView"));own(table,"float",nativeTSLBaseBinding("float"));own(table,"vec2",nativeTSLBaseBinding("vec2"));
 auto forwardCallback=JSValue::function(dHdxyBody,{table},"",1);forwardCallback.setFunctionSourceText(BumpMapNodeSource::dHdxyCallbackSource);own(table,"dHdxy_fwd",nativeTSLBaseBinding("Fn").call({}, {forwardCallback}));
 auto perturbCallback=JSValue::function(perturbBody,{nativeFrontFacingNodeBinding("faceDirection")},"",1);perturbCallback.setFunctionSourceText(BumpMapNodeSource::perturbNormalCallbackSource);own(table,"perturbNormalArb",nativeTSLBaseBinding("Fn").call({}, {perturbCallback}));
 auto ctor=makeConstructor("BumpMapNode",1,+[](const JSValue& target,const Args& a,const Args& c)->JSValue{const auto scale=arg(a,1,nullptr);auto self=superConstruct(target,c,{"vec3"});self.set("textureNode",arg(a,0));self.set("scaleNode",scale);return self;},{{"setup",1}},setup,{table});
 ctor.setFunctionSourceText(BumpMapNodeSource::classSource);auto method=ctor.get("prototype").get("setup");method.setFunctionSourceText(BumpMapNodeSource::setupSource);auto getter=ctor.getOwnPropertyDescriptor("type")->getter.value();getter.setFunctionSourceText(BumpMapNodeSource::typeSource);
 own(bindings,"default",ctor);own(bindings,"bumpMap",invoke(nativeTSLBaseBinding("nodeProxy").call({}, {ctor}),"setParameterLength")({1,2}));
 auto module=JSValue::object();own(module,"bumpMap",bindings.get("bumpMap"));own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBumpMapNodeConstructor(){return nativeBumpMapNodeBinding("default");}
JSValue nativeBumpMapNodeBinding(const std::string& name){const auto keeper=nativeBumpMapNodeModule();return lexical().lock()->get(name);}

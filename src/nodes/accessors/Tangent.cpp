#include "nodes/accessors/Tangent.h"

Tangent::Source Tangent::tangentSource(bool buildingVertexStage, bool geometryHasTangent) {
    // The attribute is used when the build is for the vertex stage, since the attribute is
    // available there, or when the geometry actually holds one.
    if (buildingVertexStage || geometryHasTangent) return Source::Attribute;
    return Source::Frame;
}

bool Tangent::negatesOnBackSide(bool flatShadingIsTrue) {
    // The source asks whether flat shading is not exactly true, so anything but a plain true
    // turns the tangent over, including a builder that answers nothing.
    return !flatShadingIsTrue;
}

#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/TangentUtils.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue tangentBody(const JSValue&,const Args& args,const Args& c){
 const auto builder=arg(args,0),table=c[0];auto node=JSValue{};
 if(builder.get("subBuildFn").strictEquals("VERTEX")||invoke(builder.get("geometry"),"hasAttribute")({"tangent"}).toBoolean()){
  const auto multiply=invoke(table.get("modelViewMatrix"),"mul");
  node=invoke(invoke(multiply({table.get("vec4").call({}, {table.get("tangentLocal"),0})}).get("xyz"),"toVarying")({"v_tangentView"}),"normalize")();
 }else node=table.get("tangentViewFrame");
 if(!invoke(builder,"isFlatShading")().strictEquals(true))node=table.get("negateOnBackSide").call({}, {node});return node;
}
}
JSValue nativeTangentModule(){
 using namespace TSLFunctionRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Tangent module");
 const auto attribute=nativeAttributeNodeModule(),camera=nativeCameraModule(),model=nativeModelNodeModule(),tsl=nativeTSLBase(),utils=nativeTangentUtilsModule(),facing=nativeFrontFacingNodeModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 own(table,"modelViewMatrix",nativeModelNodeBinding("modelViewMatrix"));own(table,"vec4",nativeTSLBaseBinding("vec4"));own(table,"tangentViewFrame",nativeTangentUtilsBinding("tangentViewFrame"));own(table,"negateOnBackSide",nativeFrontFacingNodeBinding("negateOnBackSide"));
 own(bindings,"tangentGeometry",nativeAttributeNodeBinding("attribute").call({}, {"tangent","vec4"}));
 const auto local=invoke(bindings.get("tangentGeometry").get("xyz"),"toVar")({"tangentLocal"});own(bindings,"tangentLocal",local);own(table,"tangentLocal",local);
 auto callback=JSValue::function(tangentBody,{table},"",1);callback.setFunctionSourceText(TangentSource::tangentViewCallbackSource);
 auto scopes=JSValue::array();own(scopes,"0","NORMAL");own(scopes,"1","VERTEX");
 const auto once=invoke(nativeTSLBaseBinding("Fn").call({}, {callback,"vec3"}),"once")({scopes});
 own(bindings,"tangentView",invoke(once.call(),"toVar")({"tangentView"}));
 own(bindings,"tangentWorld",invoke(invoke(invoke(invoke(bindings.get("tangentView"),"transformDirection")({nativeCameraBinding("cameraWorldMatrix")}),"toVarying")({"v_tangentWorld"}),"normalize")(),"toVar")({"tangentWorld"}));
 auto module=JSValue::object();for(const auto* name:{"tangentGeometry","tangentLocal","tangentView","tangentWorld"})own(module,name,bindings.get(name));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeTangentBinding(const std::string& name){const auto keeper=nativeTangentModule();return lexical().lock()->get(name);}

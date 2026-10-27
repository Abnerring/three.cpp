// Port of three.js ReflectVector.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/accessors/ReflectVector.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/MaterialProperties.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <algorithm>
namespace {JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeReflectVectorModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ReflectVector module");
    const auto camera=nativeCameraModule(),normal=nativeNormalModule(),position=nativePositionModule(),properties=nativeMaterialPropertiesModule();
    const auto cameraWorld=nativeCameraBinding("cameraWorldMatrix"),normalView=nativeNormalBinding("normalView"),direction=nativePositionBinding("positionViewDirection");
    auto bindings=JSValue::object(nullptr);
    own(bindings,"reflectView",invoke(invoke(direction,"negate")(),"reflect")({normalView}));
    own(bindings,"refractView",invoke(invoke(direction,"negate")(),"refract")({normalView,nativeMaterialPropertiesBinding("materialRefractionRatio")}));
    own(bindings,"reflectVector",invoke(invoke(bindings.get("reflectView"),"transformDirection")({cameraWorld}),"toVar")({"reflectVector"}));
    own(bindings,"refractVector",invoke(invoke(bindings.get("refractView"),"transformDirection")({cameraWorld}),"toVar")({"refractVector"}));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeReflectVectorBinding(const std::string& name){const auto keeper=nativeReflectVectorModule();return lexical().lock()->get(name);}

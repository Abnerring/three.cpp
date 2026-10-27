// Port of three.js MaterialProperties.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/accessors/MaterialProperties.h"
#include "math/Matrix4.h"
#include "nodes/core/UniformNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
#include "nodes/accessors/MaterialPropertiesSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue reference(const JSValue&,const Args& args,const Args&){return arg(args,0).get("material");}
JSValue update(const JSValue&,const Args& args,const Args& c){
    const auto frame=arg(args,0),material=frame.get("material");
    if(c[0].strictEquals("ratio"))return material.get("refractionRatio");
    // Object destructuring reads both fields before evaluating the body.
    const auto scene=frame.get("scene");
    if(c[0].strictEquals("intensity"))return material.get("envMap").toBoolean()?material.get("envMapIntensity"):scene.get("environmentIntensity");
    JSValue environment=scene.get("environment");
    bool hasSceneEnvironment=!environment.isNull();
    if(!hasSceneEnvironment){environment=scene.get("environmentNode");if(environment.toBoolean())hasSceneEnvironment=scene.get("environmentNode").get("isNode").toBoolean();}
    const auto rotation=hasSceneEnvironment&&material.get("envMap").isNull()?scene.get("environmentRotation"):material.get("envMapRotation");
    const auto matrix=c[1];
    if(rotation.toBoolean())invoke(invoke(matrix,"makeRotationFromEuler")({rotation}),"transpose")();
    else invoke(matrix,"identity")();
    return matrix;
}
}
JSValue nativeMaterialPropertiesModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialProperties module");
    const auto matrixConstructor=nativeMatrix4Constructor(),uniform=nativeUniformFactory();
    const auto matrix=matrixConstructor.construct();auto bindings=JSValue::object(nullptr);
    for(const auto& [name,kind]:std::initializer_list<std::pair<const char*,const char*>>{{"materialRefractionRatio","ratio"},{"materialEnvIntensity","intensity"},{"materialEnvRotation","rotation"}}){
        const auto value=std::string(kind)=="rotation"?matrixConstructor.construct():JSValue(std::string(kind)=="ratio"?0:1);
        auto node=uniform.call({}, {value});
        const std::string key=kind;
        auto ref=key=="rotation"?JSValue::constructor(reference,{},"",1,JSValue::ConstructorKind::Callable):JSValue::function(reference,{},"",1);
        ref.setFunctionSourceText(key=="ratio"?MaterialPropertiesSource::materialRefractionRatio_reference:key=="intensity"?MaterialPropertiesSource::materialEnvIntensity_reference:MaterialPropertiesSource::materialEnvRotation_reference);
        node=invoke(node,"onReference")({ref});
        auto updated=key=="ratio"?JSValue::function(update,{kind,matrix},"",1):JSValue::constructor(update,{kind,matrix},"",1,JSValue::ConstructorKind::Callable);
        updated.setFunctionSourceText(key=="ratio"?MaterialPropertiesSource::materialRefractionRatio_update:key=="intensity"?MaterialPropertiesSource::materialEnvIntensity_update:MaterialPropertiesSource::materialEnvRotation_update);
        node=invoke(node,"onObjectUpdate")({updated});own(bindings,name,node);
    }
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMaterialPropertiesBinding(const std::string& name){const auto keeper=nativeMaterialPropertiesModule();return lexical().lock()->get(name);}

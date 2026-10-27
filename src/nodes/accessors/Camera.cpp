#include "nodes/accessors/Camera.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/BuiltinNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Vector3.h"
#include <algorithm>
#include <array>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical() {static thread_local JSWeakValue weak;return weak;}
JSValue imports() {
    static thread_local JSWeakValue weak;if(const auto table=weak.lock();table&&table->isObject())return *table;
    const auto uniform=nativeUniformNodeConstructor(),render=nativeUniformGroupNodeBinding("renderGroup"),shared=nativeUniformGroupNodeBinding("sharedUniformGroup"),vector=nativeVector3Constructor(),tsl=nativeTSLBase(),array=nativeUniformArrayNodeModule(),builtin=nativeBuiltinNodeModule(),screen=nativeScreenNodeModule();
    auto table=JSValue::object(nullptr); own(table,"uniform",nativeUniformFactory());own(table,"renderGroup",render);own(table,"sharedUniformGroup",shared);own(table,"Vector3",vector);
    own(table,"Fn",nativeTSLBaseBinding("Fn"));own(table,"vec4",nativeTSLBaseBinding("vec4"));own(table,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));own(table,"builtin",nativeBuiltinNodeBinding("builtin"));own(table,"screenSize",nativeScreenNodeBinding("screenSize"));
    for(const auto* name:{"cameraProjectionMatrix","cameraProjectionMatrixInverse","cameraViewMatrix","cameraWorldMatrix","cameraNormalMatrix","cameraPosition","cameraViewport"}) {
        own(table,(std::string(name)+"Base").c_str(),nullptr);own(table,(std::string(name)+"Array").c_str(),nullptr);
    }
    table.retainForHeapLifetime();weak=JSWeakValue(table);return table;
}
JSValue update(const JSValue&,const Args& args,const Args& captures) {
    const auto camera=arg(args,0).get("camera");const auto kind=captures[0];
    if(kind.strictEquals("positionBase")) return invoke(arg(args,1).get("value"),"setFromMatrixPosition")({camera.get("matrixWorld")});
    if(kind.strictEquals("positionArray")) {
        const auto subCameras=camera.get("cameras"),array=arg(args,1).get("array"),length=subCameras.get("length");
        for(double i=0;less(i,length);++i) invoke(array.get(JSValue(i)),"setFromMatrixPosition")({subCameras.get(JSValue(i)).get("matrixWorld")});
        return {};
    }
    return camera.get(kind.toPropertyKey());
}
JSValue callback(const char* kind,unsigned arity=1) {return JSValue::function(update,{kind},"",arity);}
JSValue body(const JSValue&,const Args& args,const Args& captures) {
    const auto camera=arg(args,0).get("camera"),name=captures[1],property=captures[2],plural=captures[3];auto table=captures[0];
    const auto baseKey=name.string()+"Base",arrayKey=name.string()+"Array";
    const bool position=name.strictEquals("cameraPosition"),viewport=name.strictEquals("cameraViewport");
    if(camera.get("isArrayCamera").toBoolean()&&greater(camera.get("cameras").get("length"),0)) {
        auto values=JSValue::array({});
        if(position) {
            const auto length=camera.get("cameras").get("length");
            for(double i=0;less(i,length);++i) invoke(values,"push")({table.get("Vector3").construct()});
        } else each(camera.get("cameras"),[&](const JSValue& subCamera){invoke(values,"push")({subCamera.get(property.toPropertyKey())});});
        auto array=table.get(arrayKey);
        if(array.isNull()) {
            const auto uniform=viewport?table.get("uniformArray").call({}, {values,"vec4"}):table.get("uniformArray").call({}, {values});
            array=invoke(invoke(uniform,"setGroup")({table.get("renderGroup")}),"setName")({plural});
            if(position) array=invoke(array,"onRenderUpdate")({callback("positionArray",2)});
            table.set(arrayKey,array);
        } else array.set("array",values);
        const auto element=invoke(array,"element");
        const auto index=!viewport&&camera.get("isMultiViewCamera").toBoolean()?table.get("builtin").call({}, {"gl_ViewID_OVR"}):table.get("cameraIndex");
        return element({index});
    }
    auto base=table.get(baseKey);
    if(base.isNull()) {
        if(viewport) {
            const auto size=table.get("screenSize"),x=size.get("x"),y=size.get("y");
            base=invoke(table.get("vec4").call({}, {0,0,x,y}),"toConst")({"cameraViewport"});
        } else {
            const auto value=position?table.get("Vector3").construct():camera.get(property.toPropertyKey());
            base=invoke(invoke(invoke(table.get("uniform").call({}, {value}),"setName")({name}),"setGroup")({table.get("renderGroup")}),"onRenderUpdate")({position?callback("positionBase",2):JSValue::function(update,{property},"",1)});
        }
        table.set(baseKey,base);
    }
    return base;
}
}
JSValue nativeCameraModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Camera exports");
    const auto table=imports();auto bindings=JSValue::object(nullptr);
    auto index=invoke(table.get("uniform").call({}, {0,"uint"}),"setName")({"u_cameraIndex"});
    index=invoke(index,"setGroup")({table.get("sharedUniformGroup").call({}, {"cameraIndex"})});
    index=invoke(index,"toVarying")({"v_cameraIndex"});own(bindings,"cameraIndex",index);own(table,"cameraIndex",index);
    for(const auto& [name,key]:std::initializer_list<std::pair<const char*,const char*>>{{"cameraNear","near"},{"cameraFar","far"}}) {
        const auto value=invoke(invoke(invoke(table.get("uniform").call({}, {"float"}),"setName")({name}),"setGroup")({table.get("renderGroup")}),"onRenderUpdate")({callback(key)});own(bindings,name,value);
    }
    for(const auto& spec:std::initializer_list<std::array<const char*,3>>{
        {"cameraProjectionMatrix","projectionMatrix","cameraProjectionMatrices"},{"cameraProjectionMatrixInverse","projectionMatrixInverse","cameraProjectionMatricesInverse"},
        {"cameraViewMatrix","matrixWorldInverse","cameraViewMatrices"},{"cameraWorldMatrix","matrixWorld","cameraWorldMatrices"},{"cameraNormalMatrix","normalMatrix","cameraNormalMatrices"},
        {"cameraPosition","matrixWorld","cameraPositions"},{"cameraViewport","viewport","cameraViewports"}}) {
        const auto fn=invoke(table.get("Fn").call({}, {JSValue::function(body,{table,spec[0],spec[1],spec[2]},"",1)}),"once")();own(bindings,spec[0],fn.call());
    }
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeCameraBinding(const std::string& name) {const auto keeper=nativeCameraModule();return lexical().lock()->get(name);}

bool CameraNames::indexHasPrefix() { return std::string(kIndexName).rfind("u_",0)==0; }
bool CameraNames::usesMatrixArray(bool isArrayCamera,std::size_t subCameraCount) { return isArrayCamera&&subCameraCount>0; }

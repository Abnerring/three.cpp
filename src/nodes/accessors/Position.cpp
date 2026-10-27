// Ported from pinned three.js Position.js; see THREEJS-LICENSE.
#include "nodes/accessors/Position.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Camera.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue scopeArray(const Args& scopes){auto value=JSValue::array();for(std::size_t i=0;i<scopes.size();++i)own(value,std::to_string(i).c_str(),scopes[i]);return value;}
JSValue body(const JSValue&,const Args& args,const Args& captures){
    const auto name=captures[0],imports=captures[1],bindings=captures[2],builder=arg(args,0);
    if(name.strictEquals("clipSpace")){
        if(!builder.get("shaderStage").strictEquals("fragment")){
            ThreeUtils::warnOnce({"TSL: `clipSpace` is only available in fragment stage."});
            return imports.get("vec4").call();
        }
        return invoke(builder.get("context").get("clipSpace"),"toVarying")({"v_clipSpace"});
    }
    if(name.strictEquals("positionWorld")){
        const auto value=invoke(imports.get("modelWorldMatrix"),"mul")({bindings.get("positionLocal")}).get("xyz");
        return invoke(value,"toVarying")({invoke(builder,"getSubBuildProperty")({"v_positionWorld"})});
    }
    if(name.strictEquals("positionWorldDirection")){
        const auto vertex=invoke(invoke(bindings.get("positionLocal"),"transformDirection")({imports.get("modelWorldMatrix")}),"toVarying")({"v_positionWorldDirection"});
        return invoke(invoke(vertex,"normalize")(),"toVar")({"positionWorldDirection"});
    }
    if(name.strictEquals("positionView")){
        if(builder.get("shaderStage").strictEquals("fragment")&&builder.get("material").get("vertexNode").toBoolean()){
            const auto value=invoke(imports.get("cameraProjectionMatrixInverse"),"mul")({bindings.get("clipSpace")});
            return invoke(invoke(value.get("xyz"),"div")({value.get("w")}),"toVar")({"positionView"});
        }
        return invoke(invoke(builder.get("context"),"setupPositionView")(),"toVarying")({"v_positionView"});
    }
    if(name.strictEquals("positionViewDirection")){
        const auto output=builder.get("camera").get("isOrthographicCamera").toBoolean()?imports.get("vec3").call({}, {0,0,1}):invoke(invoke(invoke(bindings.get("positionView"),"negate")(),"toVarying")({"v_positionViewDirection"}),"normalize")();
        return invoke(output,"toVar")({"positionViewDirection"});
    }
    throw std::logic_error("Unknown Position callback");
}
JSValue once(const char* name,const JSValue& imports,const JSValue& bindings,const Args& scopes={},bool typed=true,unsigned arity=1){
    const auto callback=JSValue::function(body,{name,imports,bindings},"",arity);
    const auto fn=typed?imports.get("Fn").call({}, {callback,"vec3"}):imports.get("Fn").call({}, {callback});
    const auto callOnce=invoke(fn,"once");return (scopes.empty()?callOnce():callOnce({scopeArray(scopes)})).call();
}
}
JSValue nativePositionModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Position module");
    const auto attribute=nativeAttributeNodeModule(),tsl=nativeTSLCore(),model=nativeModelNodeModule(),camera=nativeCameraModule();
    auto imports=JSValue::object(nullptr),bindings=JSValue::object(nullptr);own(imports,"attribute",nativeAttributeNodeBinding("attribute"));
    for(const auto* name:{"Fn","vec3","vec4"})own(imports,name,nativeTSLCoreBinding(name));
    own(imports,"modelWorldMatrix",nativeModelNodeBinding("modelWorldMatrix"));own(imports,"cameraProjectionMatrixInverse",nativeCameraBinding("cameraProjectionMatrixInverse"));
    own(bindings,"clipSpace",once("clipSpace",imports,bindings,{},false));
    own(bindings,"positionGeometry",imports.get("attribute").call({}, {"position","vec3"}));
    own(bindings,"positionLocal",invoke(bindings.get("positionGeometry"),"toVarying")({"positionLocal"}));
    own(bindings,"positionPrevious",invoke(bindings.get("positionGeometry"),"toVarying")({"positionPrevious"}));
    own(bindings,"positionWorld",once("positionWorld",imports,bindings,{"POSITION"}));
    own(bindings,"positionWorldDirection",once("positionWorldDirection",imports,bindings,{"POSITION"},true,0));
    own(bindings,"positionView",once("positionView",imports,bindings,{"POSITION","VERTEX"}));
    own(bindings,"positionViewDirection",once("positionViewDirection",imports,bindings,{"POSITION"}));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativePositionBinding(const std::string& name){const auto keeper=nativePositionModule();return lexical().lock()->get(name);}

#include "nodes/accessors/Normal.h"

std::string Normal::missingAttributeWarning() {
    return "TSL: Vertex attribute \"normal\" not found on geometry.";
}

Normal::LocalFallback Normal::localFallback() {
    // Up, so that a surface with no normal reads as facing the sky rather than as failing.
    LocalFallback fallback;
    fallback.x = 0;
    fallback.y = 1;
    fallback.z = 0;
    return fallback;
}

bool Normal::usesFlatNormalViewGeometry(bool flatShadingIsTruthy) {
    // The loose test the module uses for the view space normal.
    return flatShadingIsTruthy;
}

bool Normal::addsWorldGeometryVarying(bool flatShadingIsTrue) {
    // The strict test it uses for the world space one, eight lines later: only exactly true
    // suppresses the varying, so a builder answering nothing still adds it.
    return !flatShadingIsTrue;
}

#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "utils.h"
#include <algorithm>

namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue scopeArray(const Args& scopes){auto value=JSValue::array();for(std::size_t i=0;i<scopes.size();++i)own(value,std::to_string(i).c_str(),scopes[i]);return value;}
JSValue body(const JSValue&,const Args& args,const Args& captures){
    const auto name=captures[0],imports=captures[1],bindings=captures[2];
    if(name.strictEquals("transformNormal")){
        auto iterator=getJSIterator(arg(args,0));const auto firstValue=stepJSIterator(iterator);const auto normal=firstValue?*firstValue:JSValue{};
        const auto secondValue=iterator.done?std::optional<JSValue>{}:stepJSIterator(iterator);auto matrix=secondValue?*secondValue:JSValue{};
        if(matrix.isUndefined())matrix=imports.get("modelWorldMatrix");if(!iterator.done)closeJSIterator(iterator);
        const auto normalMatrix=invoke(invoke(imports.get("mat3").call({}, {matrix}),"inverse")(),"transpose")();
        return invoke(invoke(normalMatrix,"mul")({normal}),"normalize")();
    }
    if(name.strictEquals("transformNormalToView")){
        const auto normal=first(arg(args,0)),builder=arg(args,1),matrix=builder.get("context").get("modelNormalViewMatrix");
        if(matrix.toBoolean())return invoke(normal,"transformNormalByViewMatrix")({matrix});
        const auto transformed=invoke(imports.get("modelNormalMatrix"),"mul")({normal});
        return invoke(transformed,"transformNormalByViewMatrix")({imports.get("cameraViewMatrix")});
    }
    const auto builder=arg(args,0);
    if(name.strictEquals("normalLocal")){
        if(invoke(builder.get("geometry"),"hasAttribute")({"normal"}).strictEquals(false)){
            ThreeUtils::warn({"TSL: Vertex attribute \"normal\" not found on geometry."});return imports.get("vec3").call({}, {0,1,0});
        }
        return bindings.get("normalGeometry");
    }
    if(name.strictEquals("normalViewGeometry")){
        if(invoke(builder,"isFlatShading")().toBoolean())return bindings.get("normalFlat");
        return invoke(invoke(bindings.get("transformNormalToView").call({}, {bindings.get("normalLocal")}),"toVarying")({"v_normalViewGeometry"}),"normalize")();
    }
    if(name.strictEquals("normalWorldGeometry")){
        auto normal=invoke(bindings.get("normalViewGeometry"),"transformNormalByInverseViewMatrix")({imports.get("cameraViewMatrix")});
        if(!invoke(builder,"isFlatShading")().strictEquals(true))normal=invoke(normal,"toVarying")({"v_normalWorldGeometry"});
        return invoke(invoke(normal,"normalize")(),"toVar")({"normalWorldGeometry"});
    }
    if(name.strictEquals("normalView")){
        if(builder.get("subBuildFn").strictEquals("NORMAL")||builder.get("subBuildFn").strictEquals("VERTEX")){
            auto node=bindings.get("normalViewGeometry");if(!invoke(builder,"isFlatShading")().strictEquals(true))node=imports.get("negateOnBackSide").call({}, {node});return node;
        }
        const auto setup=invoke(builder.get("context"),"setupNormal")();const auto context=invoke(setup,"context");auto properties=JSValue::object();own(properties,"getUV",nullptr);own(properties,"getTextureLevel",nullptr);return context({properties});
    }
    if(name.strictEquals("clearcoatNormalView")){
        const auto subBuildFn=builder.get("subBuildFn"),context=builder.get("context");
        if(subBuildFn.strictEquals("NORMAL")||subBuildFn.strictEquals("VERTEX"))return bindings.get("normalView");
        const auto callContext=invoke(invoke(context,"setupClearcoatNormal")(),"context");auto properties=JSValue::object();own(properties,"getUV",nullptr);own(properties,"getTextureLevel",nullptr);return callContext({properties});
    }
    throw std::logic_error("Unknown Normal callback");
}
JSValue fn(const char* name,const JSValue& imports,const JSValue& bindings,unsigned arity=1,bool typed=true){
    const auto callback=JSValue::function(body,{name,imports,bindings},"",arity);return typed?imports.get("Fn").call({}, {callback,"vec3"}):imports.get("Fn").call({}, {callback});
}
JSValue once(const char* name,const JSValue& imports,const JSValue& bindings,const Args& scopes={}){
    const auto invokeOnce=invoke(fn(name,imports,bindings),"once");return (scopes.empty()?invokeOnce():invokeOnce({scopeArray(scopes)})).call();
}
}
JSValue nativeNormalModule(){
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Normal module");
    const auto attribute=nativeAttributeNodeModule(),camera=nativeCameraModule(),model=nativeModelNodeModule(),tsl=nativeTSLBase(),position=nativePositionModule(),facing=nativeFrontFacingNodeModule();
    auto imports=JSValue::object(nullptr),bindings=JSValue::object(nullptr);own(imports,"attribute",nativeAttributeNodeBinding("attribute"));own(imports,"cameraViewMatrix",nativeCameraBinding("cameraViewMatrix"));
    own(imports,"modelNormalMatrix",nativeModelNodeBinding("modelNormalMatrix"));own(imports,"modelWorldMatrix",nativeModelNodeBinding("modelWorldMatrix"));own(imports,"positionView",nativePositionBinding("positionView"));
    for(const auto* name:{"mat3","vec3","Fn","addMethodChaining"})own(imports,name,nativeTSLBaseBinding(name));own(imports,"negateOnBackSide",nativeFrontFacingNodeBinding("negateOnBackSide"));
    own(bindings,"normalGeometry",imports.get("attribute").call({}, {"normal","vec3"}));
    own(bindings,"normalLocal",invoke(once("normalLocal",imports,bindings),"toVar")({"normalLocal"}));
    const auto dx=invoke(imports.get("positionView"),"dFdx")();const auto cross=invoke(dx,"cross");const auto dy=invoke(imports.get("positionView"),"dFdy")();
    own(bindings,"normalFlat",invoke(invoke(cross({dy}),"normalize")(),"toVar")({"normalFlat"}));
    own(bindings,"normalViewGeometry",invoke(once("normalViewGeometry",imports,bindings),"toVar")({"normalViewGeometry"}));
    own(bindings,"normalWorldGeometry",once("normalWorldGeometry",imports,bindings));
    own(bindings,"normalView",invoke(once("normalView",imports,bindings,{"NORMAL","VERTEX"}),"toVar")({"normalView"}));
    own(bindings,"normalWorld",invoke(invoke(bindings.get("normalView"),"transformNormalByInverseViewMatrix")({imports.get("cameraViewMatrix")}),"toVar")({"normalWorld"}));
    own(bindings,"clearcoatNormalView",invoke(once("clearcoatNormalView",imports,bindings,{"NORMAL","VERTEX"}),"toVar")({"clearcoatNormalView"}));
    own(bindings,"transformNormal",fn("transformNormal",imports,bindings,1,false));imports.get("addMethodChaining").call({}, {"transformNormal",bindings.get("transformNormal")});
    own(bindings,"transformNormalToView",fn("transformNormalToView",imports,bindings,2,false));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeNormalBinding(const std::string& name){const auto keeper=nativeNormalModule();return lexical().lock()->get(name);}

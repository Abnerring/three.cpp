#include "nodes/accessors/ModelViewProjectionNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
#include "nodes/accessors/ModelViewProjectionNodeSource.inc"
}
JSValue nativeModelViewProjectionNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ModelViewProjectionNode module");const auto tsl=nativeTSLCore();
    auto body=JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return invoke(arg(a,0).get("context"),"setupModelViewProjection")();},{},"",1);body.setFunctionSourceText(ModelViewProjectionNodeSource::arrow0);
    const auto fn=invoke(nativeTSLCoreBinding("Fn").call({}, {body,"vec4"}),"once")();const auto node=invoke(fn.call(),"toVarying")({"v_modelViewProjection"});
    auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"modelViewProjection",node);own(module,"modelViewProjection",node);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeModelViewProjectionNodeBinding(const std::string& name){const auto keeper=nativeModelViewProjectionNodeModule();return lexical().lock()->get(name);}

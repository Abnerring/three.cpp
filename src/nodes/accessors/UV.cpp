#include "nodes/accessors/UV.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeUVModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"UV module");
    const auto attribute=nativeAttributeNodeBinding("attribute");
    const auto uv=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto index=arg(args,0,0);const auto suffix=greater(index,0)?index:JSValue("");const auto name=add("uv",suffix);return c[0].call({}, {name,"vec2"});
    },{attribute},"uv",0);
    auto bindings=JSValue::object(nullptr);own(bindings,"uv",uv);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"uv",uv);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeUVBinding(const std::string& name){const auto keeper=nativeUVModule();return lexical().lock()->get(name);}

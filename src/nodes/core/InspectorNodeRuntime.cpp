// Ported from the pinned three.js InspectorNode.js; see THREEJS-LICENSE.
#include "nodes/core/InspectorNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "renderers/common/InspectorBase.h"
#include "utils.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0],builder=arg(args,0);
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("getName")){const auto value=self.get("name");return value.toBoolean()?value:self.get("node").get("name");}
    if(name.strictEquals("update")){invoke(builder.get("renderer").get("inspector"),"inspect")({self});return {};}
    if(name.strictEquals("generateNodeType"))return invoke(self.get("node"),"getNodeType")({builder});
    if(name.strictEquals("setup")){
        auto node=self.get("node");
        if(builder.get("context").get("inspector").strictEquals(true)&&!self.get("callback").isNull())node=invoke(self,"callback")({node});
        if(!builder.get("renderer").get("backend").get("isWebGPUBackend").strictEquals(true)&&!builder.get("renderer").get("inspector").get("constructor").sameValue(captures[2]))
            ThreeUtils::warnOnce({"TSL: \".toInspector()\" is only available with WebGPU."});
        return node;
    }
    throw std::logic_error("Unknown InspectorNode method");
}
}
JSValue nativeInspectorNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto inspector=nativeInspectorBaseConstructor(),stages=nativeNodeConstantsBinding("NodeUpdateType");
    auto ctor=makeConstructor("InspectorNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures);self.set("node",arg(args,0));self.set("name",arg(args,1,""));self.set("callback",arg(args,2,nullptr));
        self.set("updateType",nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));self.set("isInspectorNode",true);return self;
    },{{"isCacheable",0},{"getName",0},{"update",1},{"generateNodeType",1},{"setup",1}},method,{inspector});
    weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeInspectorNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"InspectorNode exports");
    const auto tsl=nativeTSLCore(),constructor=nativeInspectorNodeConstructor(),nodeObject=nativeTSLCoreBinding("nodeObject");auto bindings=JSValue::object(nullptr);own(bindings,"default",constructor);
    own(bindings,"inspect",JSValue::constructor(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto node=captures[1].call({}, {arg(args,0)});const auto before=invoke(node,"before");const auto inspector=captures[0].construct({node,arg(args,1,""),arg(args,2,nullptr)});return before({inspector});
    },{constructor,nodeObject},"inspect",1,JSValue::ConstructorKind::Callable));
    nativeTSLCoreBinding("addMethodChaining").call({}, {"toInspector",bindings.get("inspect")});
    auto module=JSValue::object();for(const auto* name:{"default","inspect"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeInspectorNodeBinding(const std::string& name){const auto keeper=nativeInspectorNodeModule();return lexical().lock()->get(name);}
InspectorNode::InspectorNode(const JSValue& node,const JSValue& name,const JSValue& callback):object_(constructor().construct({node,name,callback})){}

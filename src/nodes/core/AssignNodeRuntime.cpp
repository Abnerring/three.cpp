#include "nodes/core/AssignNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
JSValue text(const JSValue& value) { return value.toString(); }
JSValue assignContext() { auto context=JSValue::object();context.set("assign",true);return context; }
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver;const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="generateNodeType")return arg(args,1).strictEquals("void")?JSValue("void"):invoke(self.get("targetNode"),"getNodeType")({builder});
    if(name=="needsSplitAssign") {
        const auto target=self.get("targetNode");
        if(invoke(builder,"isAvailable")({"swizzleAssign"}).strictEquals(false)&&target.get("isSplitNode").toBoolean()&&greater(target.get("components").get("length"),1)) {
            const auto getLength=invoke(builder,"getTypeLength");const auto length=getLength({invoke(target.get("node"),"getNodeType")({builder})});
            const auto joined=invoke(captures[2],"join")({""});const auto prefix=invoke(joined,"slice")({0,length});
            return !prefix.strictEquals(target.get("components"));
        }
        return false;
    }
    if(name=="setup") {
        const auto target=self.get("targetNode"),source=self.get("sourceNode");
        const auto scope=invoke(target,"getScope")();auto data=invoke(builder,"getDataFromNode")({scope});data.set("assign",true);
        auto properties=invoke(builder,"getNodeProperties")({self});properties.set("sourceNode",source);
        const auto context=invoke(target,"context");properties.set("targetNode",context({assignContext()}));return {};
    }
    if(name=="generate") {
        const auto properties=invoke(builder,"getNodeProperties")({self});const auto targetNode=properties.get("targetNode"),sourceNode=properties.get("sourceNode");
        const auto needsSplit=invoke(self,"needsSplitAssign")({builder});const auto target=invoke(targetNode,"build")({builder});
        const auto targetType=invoke(targetNode,"getNodeType")({builder});const auto source=invoke(sourceNode,"build")({builder,targetType});
        const auto sourceType=invoke(sourceNode,"getNodeType")({builder});auto data=invoke(builder,"getDataFromNode")({self});
        const auto output=arg(args,1);JSValue snippet;
        if(!data.get("propertyName").isUndefined()) {if(!output.strictEquals("void"))snippet=target;}
        else if(needsSplit.toBoolean()) {
            const auto sourceVar=invoke(builder,"getVarFromNode")({self,nullptr,targetType});const auto property=invoke(builder,"getPropertyName")({sourceVar});
            const auto addLine=invoke(builder,"addLineFlowCode");const auto propertyText=text(property).string();const auto sourceText=text(source).string();
            addLine({propertyText+" = "+sourceText,self});
            const auto split=targetNode.get("node");const auto context=invoke(split.get("node"),"context");const auto splitTarget=context({assignContext()});
            const auto root=invoke(splitTarget,"build")({builder});
            for(double i=0;less(i,split.get("components").get("length"));++i) {
                const auto component=split.get("components").get(JSValue(i));const auto line=invoke(builder,"addLineFlowCode");
                const auto rootText=text(root).string();const auto componentText=text(component).string();const auto propertyText=text(property).string();
                line({rootText+"."+componentText+" = "+propertyText+"[ "+JSValue(i).toString()+" ]",self});
            }
            if(!output.strictEquals("void"))snippet=target;
        } else {
            const auto targetText=text(target).string();const auto sourceText=text(source).string();snippet=targetText+" = "+sourceText;
            if(output.strictEquals("void")||sourceType.strictEquals("void")) {
                invoke(builder,"addLineFlowCode")({snippet,self});if(!output.strictEquals("void"))snippet=target;
            }
        }
        data.set("propertyName",target);return invoke(builder,"format")({snippet,targetType,output});
    }
    throw std::logic_error("Unknown AssignNode method");
}
}
JSValue nativeAssignNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto components=nativeNodeConstantsBinding("vectorComponents");
    auto constructor=makeConstructor("AssignNode",2,+[](const JSValue& newTarget,const Args& args,const Args& captures)->JSValue {
        auto self=superConstruct(newTarget,captures);self.set("targetNode",arg(args,0));self.set("sourceNode",arg(args,1));self.set("isAssignNode",true);return self;
    },{{"isCacheable",0},{"generateNodeType",2},{"needsSplitAssign",1},{"setup",1},{"generate",2}},method,{components});weak=JSWeakValue(constructor);return constructor;
}
namespace {JSWeakValue& assignBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeAssignNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"AssignNode exports");
    const auto tsl=nativeTSLCore(),constructor=nativeAssignNodeConstructor();
    const auto factory=nativeTSLCoreBinding("nodeProxy").call({}, {constructor});const auto assign=invoke(factory,"setParameterLength")({2});
    nativeTSLCoreBinding("addMethodChaining").call({}, {"assign",assign});auto exports=JSValue::object();own(exports,"assign",assign);own(exports,"default",constructor);
    auto bindings=JSValue::object(nullptr);for(const auto& name:exports.ownPropertyNames())own(bindings,name.c_str(),exports.get(name));
    bindings.retainForHeapLifetime();assignBindings()=JSWeakValue(bindings);
    exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
JSValue nativeAssignNodeBinding(const std::string& name){const auto keeper=nativeAssignNodeModule();return assignBindings().lock()->get(name);}
AssignNode::AssignNode(const JSValue& target,const JSValue& source):object_(constructor().construct({target,source})){}

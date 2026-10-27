// Ported from the pinned three.js AttributeNode.js; see THREEJS-LICENSE.
#include "nodes/core/AttributeNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/core/VaryingNode.h"
#include "utils.h"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],builder=arg(args,0);
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("getHash"))return invoke(self,"getAttributeName")({builder});
    if(name.strictEquals("getAttributeName"))return self.get("_attributeName");
    if(name.strictEquals("setAttributeName")){self.set("_attributeName",builder);return self;}
    if(name.strictEquals("generateNodeType")){
        auto type=self.get("nodeType");
        if(type.isNull()){
            const auto attributeName=invoke(self,"getAttributeName")({builder});
            if(invoke(builder,"hasGeometryAttribute")({attributeName}).toBoolean()){
                const auto attribute=invoke(builder.get("geometry"),"getAttribute")({attributeName});
                type=invoke(builder,"getTypeFromAttribute")({attribute});
            }else type="float";
        }
        return type;
    }
    if(name.strictEquals("generate")){
        const auto attributeName=invoke(self,"getAttributeName")({builder});
        const auto type=invoke(self,"getNodeType")({builder});
        const auto present=invoke(builder,"hasGeometryAttribute")({attributeName});
        if(present.strictEquals(true)){
            const auto attribute=invoke(builder.get("geometry"),"getAttribute")({attributeName});
            const auto attributeType=invoke(builder,"getTypeFromAttribute")({attribute});
            const auto nodeAttribute=invoke(builder,"getAttribute")({attributeName,attributeType});
            if(builder.get("shaderStage").strictEquals("vertex")){
                const auto format=invoke(builder,"format");const auto attributeProperty=nodeAttribute.get("name");
                return format({attributeProperty,attributeType,type});
            }
            const auto varying=captures[2].call({}, {self});return invoke(varying,"build")({builder,type});
        }
        ThreeUtils::warn({"AttributeNode: Vertex attribute \""+attributeName.toString()+"\" not found on geometry."});
        return invoke(builder,"generateConst")({type});
    }
    superMethod(self,name.string().c_str(),captures,{builder});
    if(name.strictEquals("serialize")){
        auto data=builder;data.set("global",self.get("global"));data.set("_attributeName",self.get("_attributeName"));return {};
    }
    if(name.strictEquals("deserialize")){
        self.set("global",builder.get("global"));self.set("_attributeName",builder.get("_attributeName"));return {};
    }
    throw std::logic_error("Unknown AttributeNode method");
}
}
JSValue nativeAttributeNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto varying=nativeVaryingNodeBinding("varying");
    auto ctor=makeConstructor("AttributeNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{arg(args,1,nullptr)});self.set("global",true);self.set("_attributeName",arg(args,0));return self;
    },{{"isCacheable",0},{"getHash",1},{"generateNodeType",1},{"setAttributeName",1},{"getAttributeName",0},{"generate",1},{"serialize",1},{"deserialize",1}},method,{varying});
    weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeAttributeNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"AttributeNode exports");
    const auto tsl=nativeTSLBase(),constructor=nativeAttributeNodeConstructor();auto bindings=JSValue::object(nullptr);
    own(bindings,"default",constructor);own(bindings,"attribute",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        return captures[0].construct({arg(args,0),arg(args,1,nullptr)});
    },{constructor},"attribute",1));
    auto module=JSValue::object();for(const auto* name:{"attribute","default"})own(module,name,bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeAttributeNodeBinding(const std::string& name){const auto keeper=nativeAttributeNodeModule();return lexical().lock()->get(name);}
AttributeNode::AttributeNode(const JSValue& name,const JSValue& type):object_(constructor().construct({name,type})){}
JSValue AttributeNode::setAttributeName(const JSValue& name)const{return object_.get("setAttributeName").call(object_,{name});}
JSValue AttributeNode::getAttributeName(const JSValue& builder)const{return object_.get("getAttributeName").call(object_,{builder});}

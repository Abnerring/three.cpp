// Port of three.js BufferAttributeNode.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/core/InputNode.h"
#include "nodes/core/VaryingNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/JSWeakMap.h"
#include "constants.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue getBuffer(const JSValue& library,const JSValue& value,const JSValue& itemSize){
    auto buffer=invoke(library,"get")({value});
    if(buffer.isUndefined()){buffer=InterleavedBuffer(value,itemSize).object();invoke(library,"set")({value,buffer});}
    return buffer;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto name=c[0],builder=arg(args,0),bindings=c[2];
    if(name.strictEquals("getHash")){
        JSValue id;
        if(self.get("bufferStride").strictEquals(0)&&self.get("bufferOffset").strictEquals(0)){
            auto data=invoke(builder.get("globalCache"),"getData")({self.get("value")});
            if(data.isUndefined()){data=JSValue::object();own(data,"node",self);invoke(builder.get("globalCache"),"setData")({self.get("value"),data});}
            id=data.get("node").get("id");
        }else id=self.get("id");
        return id.isSymbol()?JSValue("Symbol("+id.symbolDescription().value_or("")+")"):JSValue(id.toString());
    }
    if(name.strictEquals("generateNodeType")){
        if(self.get("bufferType").isNull()){const auto getType=invoke(builder,"getTypeFromAttribute");self.set("bufferType",getType({self.get("attribute")}));}
        return self.get("bufferType");
    }
    if(name.strictEquals("setup")){
        if(!self.get("attribute").isNull())return {};
        const auto type=invoke(self,"getNodeType")({builder});const auto itemSize=invoke(builder,"getTypeLength")({type});const auto value=self.get("value");
        const auto bufferStride=self.get("bufferStride");const auto stride=bufferStride.toBoolean()?bufferStride:itemSize;const auto offset=self.get("bufferOffset");JSValue buffer;
        if(value.get("isInterleavedBuffer").strictEquals(true))buffer=value;
        else if(value.get("isBufferAttribute").strictEquals(true))buffer=getBuffer(bindings.get("bufferLib"),value.get("array"),stride);
        else buffer=getBuffer(bindings.get("bufferLib"),value,stride);
        const auto attribute=InterleavedBufferAttribute(buffer,itemSize,offset).object();const auto setUsage=invoke(buffer,"setUsage");setUsage({self.get("usage")});
        self.set("attribute",attribute);auto assigned=self.get("attribute");assigned.set("isInstancedBufferAttribute",self.get("instanced"));return {};
    }
    if(name.strictEquals("generate")){
        const auto type=invoke(self,"getNodeType")({builder});const auto nodeName=builder.get("context").get("nodeName");
        if(!nodeName.isUndefined()){auto context=builder.get("context");if(!context.erase("nodeName"))throw JSTypeError("Cannot delete nodeName");}
        const auto attribute=invoke(builder,"getBufferAttributeFromNode")({self,type,nodeName});const auto propertyName=invoke(builder,"getPropertyName")({attribute});
        if(builder.get("shaderStage").strictEquals("vertex")||builder.get("shaderStage").strictEquals("compute")){self.set("name",propertyName);return propertyName;}
        const auto varyingName=nodeName.toBoolean()?add(nodeName,"Varying"):JSValue{};const auto varying=bindings.get("varying").call({}, {self,varyingName});
        return invoke(varying,"build")({builder,type});
    }
    if(name.strictEquals("getInputType"))return "bufferAttribute";
    if(name.strictEquals("setUsage")){
        const auto value=arg(args,0);self.set("usage",value);
        if(self.get("attribute").toBoolean()&&self.get("attribute").get("isBufferAttribute").strictEquals(true)){auto attribute=self.get("attribute");attribute.set("usage",value);}return self;
    }
    if(name.strictEquals("setInstanced")){self.set("instanced",arg(args,0));return self;}
    throw std::logic_error("Unknown BufferAttributeNode method");
}
JSValue createAttribute(const JSValue& constructor,const JSValue& bindings,const JSValue& value,const JSValue& type,const JSValue& stride,const JSValue& offset,const JSValue& usage,const JSValue& instanced){
    unsigned width=0;
    if(type.strictEquals("mat3")||(type.isNull()&&value.get("itemSize").strictEquals(9)))width=3;
    else if(type.strictEquals("mat4")||(type.isNull()&&value.get("itemSize").strictEquals(16)))width=4;
    if(width){
        const auto matrix=bindings.get(width==3?"mat3":"mat4");Args columns;
        for(unsigned i=0;i<width;++i){const auto node=constructor.construct({value,width==3?"vec3":"vec4",width*width,i*width});const auto used=invoke(node,"setUsage")({usage});columns.push_back(invoke(used,"setInstanced")({instanced}));}
        return matrix.call({},columns);
    }
    return invoke(constructor.construct({value,type,stride,offset}),"setUsage")({usage});
}
}
JSValue nativeBufferAttributeNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BufferAttributeNode");
    const auto base=nativeInputNodeConstructor(),tsl=nativeTSLCore();const auto varying=nativeVaryingNodeBinding("varying");auto holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    own(bindings,"bufferLib",createNativeWeakMap());own(bindings,"varying",varying);own(bindings,"mat3",nativeTSLCoreBinding("mat3"));own(bindings,"mat4",nativeTSLCoreBinding("mat4"));
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        const auto value=arg(args,0),type=arg(args,1,nullptr),stride=arg(args,2,0),offset=arg(args,3,0);auto self=superConstruct(target,c,{value,type});
        self.set("isBufferNode",true);self.set("bufferType",type);self.set("bufferStride",stride);self.set("bufferOffset",offset);self.set("usage",StaticDrawUsage);
        self.set("instanced",false);self.set("attribute",nullptr);self.set("global",true);
        if(value.toBoolean()&&value.get("isBufferAttribute").strictEquals(true)&&value.get("itemSize").toNumber()<=4){self.set("attribute",value);self.set("usage",value.get("usage"));self.set("instanced",value.get("isInstancedBufferAttribute"));}
        return self;
    },{holder},"BufferAttributeNode",1);
    holder.set("constructor",constructor);holder.set("bindings",bindings);constructor.setPrototype(base);auto prototype=constructor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "BufferAttributeNode";},{},"get type",0);type.configurable=true;constructor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getHash",1},{"generateNodeType",1},{"setup",1},{"generate",1},{"getInputType",0},{"setUsage",1},{"setInstanced",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,prototype,bindings},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeBufferAttributeNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BufferAttributeNode exports");
    const auto constructor=nativeBufferAttributeNodeConstructor();auto bindings=JSValue::object(nullptr);own(bindings,"default",constructor);auto factoryBindings=JSValue::object(nullptr);
    own(factoryBindings,"mat3",nativeTSLCoreBinding("mat3"));own(factoryBindings,"mat4",nativeTSLCoreBinding("mat4"));
    for(const auto* name:{"bufferAttribute","dynamicBufferAttribute","instancedBufferAttribute","instancedDynamicBufferAttribute"}){
        const std::string key=name;const auto usage=key.find("Dynamic")!=std::string::npos||key=="dynamicBufferAttribute"?DynamicDrawUsage:StaticDrawUsage;const bool instanced=key.starts_with("instanced");
        own(bindings,name,JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{return createAttribute(c[0],c[1],arg(args,0),arg(args,1,nullptr),arg(args,2,0),arg(args,3,0),c[2],c[3]);},{constructor,factoryBindings,usage,instanced},name,1));
    }
    const auto chain=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{const auto node=arg(args,0);const auto value=node.get("value"),type=node.get("bufferType");return c[0].call({}, {value,type});},{bindings.get("bufferAttribute")},"",1);
    nativeTSLCoreBinding("addMethodChaining").call({}, {"toAttribute",chain});
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBufferAttributeNodeBinding(const std::string& name){const auto keeper=nativeBufferAttributeNodeModule();return lexical().lock()->get(name);}
BufferAttributeNode::BufferAttributeNode(const JSValue& value,const JSValue& type,const JSValue& stride,const JSValue& offset):object_(constructor().construct({value,type,stride,offset})){}
JSValue BufferAttributeNode::getHash(const JSValue& builder)const{return object_.get("getHash").call(object_,{builder});}
JSValue BufferAttributeNode::generateNodeType(const JSValue& builder)const{return object_.get("generateNodeType").call(object_,{builder});}
JSValue BufferAttributeNode::setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
JSValue BufferAttributeNode::generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
JSValue BufferAttributeNode::getInputType()const{return object_.get("getInputType").call(object_);}
JSValue BufferAttributeNode::setUsage(const JSValue& value)const{return object_.get("setUsage").call(object_,{value});}
JSValue BufferAttributeNode::setInstanced(const JSValue& value)const{return object_.get("setInstanced").call(object_,{value});}

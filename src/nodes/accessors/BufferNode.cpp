#include "nodes/accessors/BufferNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& captures){
    const auto name=captures[0];if(name.strictEquals("getInputType"))return "buffer";
    if(name.strictEquals("getElementType"))return invoke(self,"getNodeType")({arg(args,0)});
    if(name.strictEquals("clearUpdateRanges")){auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}
    const auto push=invoke(self.get("updateRanges"),"push");auto range=JSValue::object();own(range,"start",arg(args,0));own(range,"count",arg(args,1));push({range});return {};
}
}
JSValue nativeBufferNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BufferNode module");const auto base=nativeUniformNodeConstructor(),tsl=nativeTSLCore();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{auto self=superConstruct(target,captures,{arg(args,0),arg(args,1)});self.set("isBufferNode",true);self.set("bufferType",arg(args,1));self.set("bufferCount",arg(args,2,0));self.set("updateRanges",JSValue::array());return self;},{holder},"BufferNode",2);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "BufferNode";},{},"get type",0);ctor.defineProperty("type",type);
    for(const auto& [key,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"addUpdateRange",2},{"clearUpdateRanges",0},{"getElementType",1},{"getInputType",0}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{key},key,arity);d.writable=d.configurable=true;prototype.defineProperty(key,d);}ctor.retainForHeapLifetime();
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"buffer",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{return c[0].construct({arg(args,0),arg(args,1),arg(args,2)});},{ctor},"buffer",3));auto module=JSValue::object();own(module,"buffer",bindings.get("buffer"));own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBufferNodeBinding(const std::string& name){const auto keeper=nativeBufferNodeModule();return lexical().lock()->get(name);}
JSValue nativeBufferNodeConstructor(){return nativeBufferNodeBinding("default");}
BufferNode::BufferNode(const JSValue& value,const JSValue& type,const JSValue& count):object_(constructor().construct({value,type,count})){}
JSValue BufferNode::addUpdateRange(const JSValue& start,const JSValue& count)const{return object_.get("addUpdateRange").call(object_,{start,count});}
JSValue BufferNode::clearUpdateRanges()const{return object_.get("clearUpdateRanges").call(object_);}
JSValue BufferNode::getElementType(const JSValue& builder)const{return object_.get("getElementType").call(object_,{builder});}
JSValue BufferNode::getInputType(const JSValue& builder)const{return object_.get("getInputType").call(object_,{builder});}

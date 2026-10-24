#include "renderers/common/Buffer.h"
#include "renderers/common/Binding.h"
#include "renderers/common/BufferUtils.h"
#include "core/NativeArray.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto name=c[0].string();
    if(name=="buffer")return self.get("_buffer");
    if(name=="updateRanges")return self.get("_updateRanges");
    if(name=="byteLength")return getFloatLength(self.get("buffer").get("byteLength"));
    if(name=="update")return true;
    if(name=="release"){self.set("_buffer",nullptr);return {};}
    if(name=="clearUpdateRanges"){auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}
    const auto ranges=self.get("updateRanges"),push=ranges.get("push");auto range=JSValue::object();
    for(const auto& [key,value]:std::initializer_list<std::pair<const char*,JSValue>>{{"start",arg(args,0)},{"count",arg(args,1)}}){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;range.defineProperty(key,d);}
    push.call(ranges,{range});return {};
}
}
JSValue nativeRendererBufferConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    auto state=JSValue::object(nullptr);const auto base=nativeBindingConstructor();
    auto ctor=JSValue::builtinConstructor([](const JSValue& target,const Args& args,const Args& c)->JSValue{
        const auto buffer=arg(args,1).isUndefined()?JSValue(nullptr):arg(args,1);
        auto self=c[0].get("constructor").getPrototype().construct({arg(args,0)},target);
        self.set("isBuffer",true);self.set("bytesPerElement",NativeArray_detail::constructor(6).get("BYTES_PER_ELEMENT"));
        self.set("_buffer",buffer);self.set("_updateRanges",JSValue::array());return self;
    },{state},"Buffer",1);
    state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    for(const auto* name:{"updateRanges","byteLength","buffer"}){JSPropertyDescriptor d;d.getter=JSValue::function(method,{name},std::string("get ")+name,0);d.configurable=true;prototype.defineProperty(name,d);}
    for(const auto* name:{"addUpdateRange","clearUpdateRanges","update","release"}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,std::string(name)=="addUpdateRange"?2:0);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}

// Direct port of three.js InstancedInterleavedBuffer.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository LICENSE.
#include "core/InstancedInterleavedBuffer.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index) { return index<args.size()?args[index]:JSValue{}; }
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver; const auto& name=captures[0].string();
    const auto base=InstancedInterleavedBuffer::prototype().getPrototype().get(name,self);
    auto result=base.call(self,{arg(args,0)});
    if(name=="copy") { self.set("meshPerAttribute",arg(args,0).get("meshPerAttribute")); return self; }
    if(name=="clone") { result.set("meshPerAttribute",self.get("meshPerAttribute")); return result; }
    result.set("isInstancedInterleavedBuffer",true); result.set("meshPerAttribute",self.get("meshPerAttribute")); return result;
}
}
void InstancedInterleavedBuffer::initializeInstance(JSValue self,JSValue array,JSValue stride,JSValue meshPerAttribute) {
    if(meshPerAttribute.isUndefined()) meshPerAttribute=1;
    initialize(self,array,stride); self.set("isInstancedInterleavedBuffer",true); self.set("meshPerAttribute",meshPerAttribute);
}
JSValue InstancedInterleavedBuffer::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue {
            initializeInstance(self,arg(args,0),arg(args,1),arg(args,2)); return {};
        },{},"InstancedInterleavedBuffer",2);
        auto result=constructor.get("prototype"); result.setPrototype(InterleavedBuffer::prototype());
        constructor.setPrototype(InterleavedBuffer::prototype().get("constructor"));
        JSPropertyDescriptor descriptor; descriptor.writable=descriptor.configurable=true;
        for(const char* name:{"copy","clone","toJSON"}) {
            descriptor.value=JSValue::function(method,{name},name,1); result.defineProperty(name,descriptor);
        }
        return result;
    }(); return value;
}
InstancedInterleavedBuffer::InstancedInterleavedBuffer(JSValue array,JSValue stride,JSValue meshPerAttribute)
    :InterleavedBuffer(JSValue::object(prototype()),Existing{}) { initializeInstance(object_,array,stride,meshPerAttribute); }

// Direct port of three.js InterleavedBuffer.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository LICENSE.
#include "core/InterleavedBuffer.h"
#include "math/MathUtils.h"
#include "constants.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) { return index<args.size()?args[index]:JSValue{}; }
JSValue call(JSValue receiver, const char* name, const Args& args = {}) {
    const auto method=receiver.get(name); return method.call(receiver,args);
}
std::string key(const JSValue& value) { return value.toPrimitive(JSValue::PreferredType::String).toString(); }
JSValue arrayKey(JSValue array) {
    if(array.get("buffer").get("_uuid").isUndefined()) {
        auto buffer=array.get("buffer"); buffer.set("_uuid",MathUtils::generateUUID());
    }
    if(array.get("byteOffset").strictEquals(0)) {
        const auto length=array.get("byteLength");
        if(length.strictEquals(array.get("buffer").get("byteLength"))) return array.get("buffer").get("_uuid");
    }
    auto result=Interpolant_detail::add(array.get("buffer").get("_uuid"),":");
    result=Interpolant_detail::add(result,array.get("byteOffset"));
    result=Interpolant_detail::add(result,":");
    return Interpolant_detail::add(result,array.get("byteLength"));
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self=receiver; const auto& name=captures[0].string();
    if(name=="onUploadCallback") return {};
    if(name=="needsUpdate") {
        if(arg(args,0).strictEquals(true)) self.set("version",self.get("version").toNumber()+1);
        return {};
    }
    if(name=="setUsage") { self.set("usage",arg(args,0)); return self; }
    if(name=="addUpdateRange") {
        const auto ranges=self.get("updateRanges"), push=ranges.get("push");
        auto range=JSValue::object(); range.set("start",arg(args,0)); range.set("count",arg(args,1));
        push.call(ranges,{range}); return {};
    }
    if(name=="clearUpdateRanges") { auto ranges=self.get("updateRanges"); ranges.set("length",0); return {}; }
    if(name=="copy") {
        const auto source=arg(args,0), constructor=source.get("array").get("constructor");
        const auto array=source.get("array"); self.set("array",constructor.construct({array}));
        self.set("count",source.get("count")); self.set("stride",source.get("stride"));
        self.set("usage",source.get("usage")); return self;
    }
    if(name=="copyAt") {
        const auto source=arg(args,1);
        const auto stride1=self.get("stride"); const auto index1=Interpolant_detail::multiply(arg(args,0),stride1);
        const auto stride2=source.get("stride"); const auto index2=Interpolant_detail::multiply(arg(args,2),stride2);
        const auto length=self.get("stride");
        for(double i=0;i<length.toNumber();++i) {
            auto destination=self.get("array"); const auto destinationKey=JSValue(index1+i).toString();
            const auto input=source.get("array"); const auto value=input.get(JSValue(index2+i).toString());
            destination.set(destinationKey,value);
        }
        return self;
    }
    if(name=="set") {
        const auto array=self.get("array"), setter=array.get("set"); auto offset=arg(args,1);
        if(offset.isUndefined()) offset=0;
        setter.call(array,{arg(args,0),offset}); return self;
    }
    if(name=="onUpload") { self.set("onUploadCallback",arg(args,0)); return self; }
    if(name=="clone"||name=="toJSON") {
        auto data=arg(args,0);
        if(data.get("arrayBuffers").isUndefined()) data.set("arrayBuffers",JSValue::object());
        const auto cacheKey=arrayKey(self.get("array"));
        auto buffers=data.get("arrayBuffers");
        if(buffers.get(key(cacheKey)).isUndefined()) {
            buffers=data.get("arrayBuffers"); const auto propertyKey=key(cacheKey);
            const auto array=self.get("array"), slicer=array.get("slice");
            const auto buffer=slicer.call(array,{0}).get("buffer");
            if(name=="clone") buffers.set(propertyKey,buffer);
            else {
                const auto constructor=nativeArray(AnimationArray(Uint32Array())).get("constructor");
                const auto integers=constructor.construct({buffer});
                auto values=JSValue::array();
                const auto length=integers.get("length").toNumber();
                for(double i=0;i<length;++i) values.set(JSValue(i).toString(),integers.get(JSValue(i).toString()));
                buffers.set(propertyKey,values);
            }
        }
        if(name=="clone") {
            const auto constructor=self.get("array").get("constructor");
            const auto buffers=data.get("arrayBuffers"); const auto storage=buffers.get(key(cacheKey));
            const auto array=constructor.construct({storage});
            const auto bufferConstructor=self.get("constructor"); const auto stride=self.get("stride");
            const auto result=bufferConstructor.construct({array,stride});
            const auto setUsage=result.get("setUsage"), usage=self.get("usage"); setUsage.call(result,{usage});
            return result;
        }
        auto json=JSValue::object(); json.set("uuid",self.get("uuid")); json.set("buffer",cacheKey);
        json.set("type",self.get("array").get("constructor").get("name"));
        json.set("stride",self.get("stride")); json.set("usage",self.get("usage")); return json;
    }
    throw JSTypeError("Unknown InterleavedBuffer method");
}
}

void InterleavedBuffer::initialize(JSValue self, JSValue array, JSValue stride) {
    self.set("isInterleavedBuffer",true); self.set("array",array); self.set("stride",stride);
    double count=0;
    if(!array.isUndefined()) {
        const auto length=array.get("length"); const auto numerator=length.toNumber(); const auto divisor=stride.toNumber();
        count=numerator/divisor;
    }
    self.set("count",count); self.set("usage",StaticDrawUsage); self.set("updateRanges",JSValue::array());
    self.set("version",0); self.set("uuid",MathUtils::generateUUID());
}
JSValue InterleavedBuffer::constructor() {
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isConstructible())return *value;
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue {
            initialize(self,arg(args,0),arg(args,1)); return {};
        },{},"InterleavedBuffer",2);
        auto result=constructor.get("prototype");
        JSPropertyDescriptor descriptor; descriptor.writable=descriptor.configurable=true;
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
            {"onUploadCallback",0},{"setUsage",1},{"addUpdateRange",2},{"clearUpdateRanges",0},{"copy",1},
            {"copyAt",3},{"set",1},{"clone",1},{"onUpload",1},{"toJSON",1}}) {
            descriptor.value=JSValue::function(method,{name},name,arity); result.defineProperty(name,descriptor);
        }
        JSPropertyDescriptor setter; setter.setter=JSValue::function(method,{"needsUpdate"},"set needsUpdate",1);
        setter.configurable=true; result.defineProperty("needsUpdate",setter);
        constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue InterleavedBuffer::prototype() { return constructor().get("prototype"); }
InterleavedBuffer::InterleavedBuffer(JSValue array,JSValue stride):object_(constructor().construct({array,stride})) {}
JSValue InterleavedBuffer::setUsage(JSValue value) { return call(object_,"setUsage",{value}); }
JSValue InterleavedBuffer::addUpdateRange(JSValue start,JSValue count) { return call(object_,"addUpdateRange",{start,count}); }
JSValue InterleavedBuffer::clearUpdateRanges() { return call(object_,"clearUpdateRanges"); }
JSValue InterleavedBuffer::copy(JSValue source) { return call(object_,"copy",{source}); }
JSValue InterleavedBuffer::copyAt(JSValue index1,JSValue source,JSValue index2) { return call(object_,"copyAt",{index1,source,index2}); }
JSValue InterleavedBuffer::set(JSValue value,JSValue offset) { return call(object_,"set",{value,offset}); }
JSValue InterleavedBuffer::clone(JSValue data) { return call(object_,"clone",{data}); }
JSValue InterleavedBuffer::onUpload(JSValue callback) { return call(object_,"onUpload",{callback}); }
JSValue InterleavedBuffer::toJSON(JSValue data) { return call(object_,"toJSON",{data}); }

#include "core/InterleavedBuffer.h"
#include "core/InstancedInterleavedBuffer.h"
#include "animation/AnimationUtils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue list(std::initializer_list<JSValue> entries) {auto value=JSValue::array();for(auto& entry:entries)value.get("push").call(value,{entry});return value;}
void push(JSValue array,JSValue value) {array.get("push").call(array,{value});}
JSValue call(JSValue value,const char* method,Args args={}) {return value.get(method).call(value,args);}
JSValue number(JSValue value) {
    if(value.isUndefined()) return "#undefined";
    if(value.type()!=JSValue::Type::Number) return value;
    const auto n=value.number(); if(std::isnan(n)) return "#NaN";
    if(std::isinf(n)) return n<0?"#-Infinity":"#Infinity";
    if(n==0&&std::signbit(n)) return "#-0";return value;
}
JSValue values(JSValue array) {auto result=JSValue::array();for(double i=0;i<array.get("length").toNumber();++i)push(result,number(array.get(JSValue(i).toString())));return result;}
JSValue keys(JSValue value) {auto result=JSValue::array();for(const auto& key:value.keys())push(result,key);return result;}
JSValue state(JSValue value) {
    auto result=JSValue::object();
    result.set("keys",keys(value));result.set("data",value.get("array").isUndefined()?JSValue("#undefined"):values(value.get("array")));
    for(const char* key:{"stride","count","usage","version","meshPerAttribute","isInterleavedBuffer","isInstancedInterleavedBuffer","needsUpdate"})result.set(key,number(value.get(key)));
    result.set("ranges",value.get("updateRanges"));return result;
}
JSValue ctor(int kind) {
    switch(kind){
    case 0:return nativeArray(Int8Array()).get("constructor");case 1:return nativeArray(Uint8Array()).get("constructor");
    case 2:return nativeArray(Int16Array()).get("constructor");case 3:return nativeArray(Uint16Array()).get("constructor");
    case 4:return nativeArray(Int32Array()).get("constructor");case 5:return nativeArray(Uint32Array()).get("constructor");
    case 6:return nativeArray(Float32Array()).get("constructor");case 7:return nativeArray(Float64Array()).get("constructor");
    default:return nativeArray(Uint8ClampedArray()).get("constructor");}
}
JSValue make(int kind,int size=12) {
    auto data=ctor(kind).construct({size});
    for(int i=0;i<size;++i)data.set(std::to_string(i),i%3==0?-double(i)*2.25:i*1.5+.25);
    data.get("buffer").set("_uuid","buffer");return data;
}
template<class F> JSValue attempt(F&& callback) {
    try {return callback();}catch(const JSRangeError&){return "RangeError";}catch(const JSTypeError&){return "TypeError";}
}
void getter(JSValue target,const char* name,JSValue log,JSValue value) {
    JSPropertyDescriptor d;d.configurable=true;d.enumerable=true;
    d.getter=JSValue::function([](const JSValue&,const Args&,const Args& capture){push(capture[0],capture[1]);return capture[2];},{log,name,value});
    target.defineProperty(name,d);
}
JSValue ordinaryCase(int kind,bool instanced) {
    const auto data=make(kind);auto source=instanced?InstancedInterleavedBuffer(data,3,2).object():InterleavedBuffer(data,3).object();
    source.set("uuid","source");auto result=JSValue::object();result.set("initial",state(source));
    const auto rangeStart=JSValue::object();auto marker=JSValue::object();marker.set("x",3);
    call(source,"addUpdateRange",{rangeStart,marker});const auto ranges=source.get("updateRanges"),range=ranges.get("0");
    result.set("rangeIdentity",list({range.get("start").sameValue(rangeStart),range.get("count").sameValue(marker)}));
    call(source,"clearUpdateRanges");result.set("rangesSame",ranges.sameValue(source.get("updateRanges")));
    for(JSValue v:{JSValue(true),JSValue(1),JSValue("true"),JSValue(false),JSValue(),JSValue(true)})source.set("needsUpdate",v);
    source.set("version","5");source.set("needsUpdate",true);call(source,"setUsage",{35048});
    auto target=instanced?InstancedInterleavedBuffer(make(kind,4),1,5).object():InterleavedBuffer(make(kind,4),1).object();
    target.set("uuid","target");target.set("version",4);call(target,"addUpdateRange",{1,2});
    const auto callback=JSValue::function([](const JSValue& self,const Args&,const Args&){auto value=self;value.set("uploaded",true);return JSValue{};});
    call(target,"onUpload",{callback});call(target,"copy",{source});
    result.set("copy",state(target));result.set("copyIdentity",list({!target.get("array").sameValue(data),!target.get("array").get("buffer").sameValue(data.get("buffer")),target.get("onUploadCallback").sameValue(callback),target.get("uuid").strictEquals("target")}));
    call(target,"onUploadCallback");result.set("callbackReceiver",target.get("uploaded"));
    auto metadata=JSValue::object();const auto cloned=call(source,"clone",{metadata});
    result.set("clone",state(cloned));result.set("cloneIdentity",list({!cloned.sameValue(source),!cloned.get("array").sameValue(data),!cloned.get("array").get("buffer").sameValue(data.get("buffer")),
        cloned.get("onUploadCallback").sameValue(InterleavedBuffer::prototype().get("onUploadCallback")),!cloned.get("uuid").sameValue(source.get("uuid"))}));
    auto jsonMeta=JSValue::object();const auto json=call(source,"toJSON",{jsonMeta});result.set("json",json);result.set("jsonKeys",keys(json));result.set("meta",jsonMeta);
    result.set("final",state(source));return result;
}
JSValue viewsCase(int kind) {
    const auto full=make(kind,16),a=call(full,"subarray",{2,10}),b=call(full,"subarray",{2,10}),c=call(full,"subarray",{3,11});
    auto meta=JSValue::object();auto first=InterleavedBuffer(a,2),second=InterleavedBuffer(b,4),third=InterleavedBuffer(c,2);
    const auto ca=first.clone(meta),cb=second.clone(meta),cc=third.clone(meta);auto result=JSValue::object();
    result.set("identity",list({!a.sameValue(b),a.get("buffer").sameValue(b.get("buffer")),!ca.get("array").sameValue(cb.get("array")),
        ca.get("array").get("buffer").sameValue(cb.get("array").get("buffer")),!ca.get("array").get("buffer").sameValue(cc.get("array").get("buffer"))}));
    result.set("cacheKeys",keys(meta.get("arrayBuffers")));ca.get("array").set("1",42);
    result.set("data",list({values(ca.get("array")),values(cb.get("array")),values(cc.get("array")),values(a)}));
    result.set("offsets",list({ca.get("array").get("byteOffset"),cb.get("array").get("byteOffset"),cc.get("array").get("byteOffset")}));
    auto jsonMeta=JSValue::object();first.uuid="first";second.uuid="second";third.uuid="third";
    result.set("json",list({first.toJSON(jsonMeta),second.toJSON(jsonMeta),third.toJSON(jsonMeta)}));result.set("meta",jsonMeta);
    return result;
}
JSValue countCase(JSValue stride) {
    auto result=JSValue::object();InterleavedBuffer source(make(6),stride);
    result.set("initial",state(source));source.stride=7;result.set("countUnchanged",number(source.count));
    const auto target=InterleavedBuffer::fromObject(source.clone(JSValue::object()));result.set("cloneCount",number(target.count));return result;
}
JSValue copyingCase() {
    auto result=JSValue::object();const auto data=nativeArray(Float32Array({1,2,3,4,5,6}));InterleavedBuffer source(data,3);
    source.copyAt(1.0/3,source,0);result.set("overlap",values(data));
    source.stride=2.5;source.copyAt(0,InterleavedBuffer(nativeArray(Float32Array({8,9,10,11,12,13})),2),.5);result.set("differentStride",values(data));
    source.copyAt(-.4,source,0);result.set("negative",values(data));
    source.set(call(data,"subarray",{0,4}),1);result.set("setOverlap",values(data));
    result.set("setRangeError",attempt([&]{return source.set(list({1,2}),6);}));
    result.set("afterError",values(data));source.set(list({JSValue(),nullptr,true,"3.5"}),0);result.set("setConversions",values(data));
    return result;
}
JSValue errorsCase() {
    auto result=JSValue::object();InterleavedBuffer absent;
    result.set("default",state(absent));result.set("null",attempt([]{return InterleavedBuffer(nullptr,2).object();}));
    result.set("noMetaClone",attempt([&]{return absent.clone();}));result.set("noMetaJSON",attempt([&]{return absent.toJSON();}));
    result.set("undefinedArrayClone",attempt([&]{return absent.clone(JSValue::object());}));
    InterleavedBuffer unaligned(nativeArray(Uint8Array({1,2,3})),1);auto meta=JSValue::object();
    unaligned.array.get("buffer").set("_uuid","small");result.set("unalignedJSON",attempt([&]{return unaligned.toJSON(meta);}));result.set("unalignedMeta",meta);
    InterleavedBuffer source(make(6),3);source.uuid="source";
    auto mixed=JSValue::object();source.clone(mixed);result.set("jsonAfterClone",source.toJSON(mixed));
    auto reverse=JSValue::object();source.toJSON(reverse);auto clone=source.clone(reverse);result.set("cloneAfterJSON",state(clone));
    source.array.get("buffer").set("_uuid","__proto__");auto polluted=JSValue::object();
    result.set("prototypeKey",state(InterleavedBuffer::fromObject(source.clone(polluted))));
    result.set("prototypeCache",keys(polluted.get("arrayBuffers")));
    InstancedInterleavedBuffer instance(make(6),3,JSValue::object());instance.copy(source);
    result.set("instanceFromBase",state(instance));instance.uuid="instance";instance.array.get("buffer").set("_uuid","instance-buffer");result.set("instanceJSON",instance.toJSON(JSValue::object()));
    return result;
}
JSValue callbackCase() {
    auto log=JSValue::array(),source=JSValue::object(),result=JSValue::object();const auto array=make(6);
    getter(source,"array",log,array);getter(source,"count",log,4);getter(source,"stride",log,3);getter(source,"usage",log,35048);
    InterleavedBuffer target;target.copy(source);result.set("copyLog",log);
    log=JSValue::array();auto data=make(6);InterleavedBuffer buffer(data,3);buffer.uuid="source";
    auto meta=JSValue::object(),cache=JSValue::object();getter(meta,"arrayBuffers",log,cache);
    buffer.clone(meta);result.set("cloneMetaLog",log);
    log=JSValue::array();auto ranges=JSValue::object();getter(buffer.object(),"updateRanges",log,ranges);
    ranges.set("push",JSValue::function([](const JSValue& self,const Args& args,const Args& captures){push(captures[0],self.sameValue(captures[1]));push(captures[0],args[0]);return JSValue{};},{log,ranges}));
    buffer.addUpdateRange(2,7);result.set("rangeLog",log);
    log=JSValue::array();InterleavedBuffer custom(make(6),3);auto customCtor=JSValue::constructor([](const JSValue&,const Args& args,const Args& captures){
        push(captures[0],"constructor");auto value=InterleavedBuffer(args[0],args[1]).object();value.set("custom",true);
        value.set("setUsage",JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures){push(captures[0],"setUsage");auto value=receiver;value.set("usage",args[0]);return value;},{captures[0]}));return value;
    },{log},"Custom",2);
    getter(custom.object(),"constructor",log,customCtor);getter(custom.object(),"stride",log,2);getter(custom.object(),"usage",log,35048);
    const auto clone=custom.clone(JSValue::object());result.set("customConstructorLog",log);result.set("custom",clone.get("custom"));result.set("customCount",clone.get("count"));
    log=JSValue::array();auto changing=JSValue::object();changing.set("reads",0);const auto array2=make(6,6);
    JSPropertyDescriptor read;read.getter=JSValue::function([](const JSValue& self,const Args&,const Args& captures){auto value=self;const auto n=self.get("reads").number();value.set("reads",n+1);push(captures[0],n);return captures[n==0?1:2];},{log,array,array2});
    changing.defineProperty("array",read);changing.set("count",99);changing.set("stride",1);changing.set("usage",1);
    target.copy(changing);result.set("changingLog",log);result.set("changingData",values(target.array));result.set("changingCount",target.count.value());
    return result;
}
JSValue bytesCase() {
    Uint32Array bits({0x7fc12345u,0xffcabcdeu,0x80000000u,0x3f800000u});
    auto floats=nativeArray(Float32Array(bits.buffer));auto integers=nativeArray(bits);floats.get("buffer").set("_uuid","bits");
    InterleavedBuffer source(floats,2),other(integers,2);auto meta=JSValue::object();
    const auto first=source.clone(meta),second=other.clone(meta);InterleavedBuffer copy;copy.copy(source);
    auto result=JSValue::object();result.set("sharedDifferentTypes",first.get("array").get("buffer").sameValue(second.get("array").get("buffer")));
    result.set("cloneBits",values(ctor(5).construct({first.get("array").get("buffer")})));
    result.set("copyBits",values(ctor(5).construct({copy.array.get("buffer")})));
    source.uuid="source";auto jsonMeta=JSValue::object();result.set("json",source.toJSON(jsonMeta));result.set("meta",jsonMeta);
    const auto buffer=floats.get("buffer");auto different=ctor(2).construct({buffer,2,5});
    InterleavedBuffer sub(different,2);auto submeta=JSValue::object();const auto cloned=sub.clone(submeta);
    result.set("subarray",values(cloned.get("array")));result.set("subarrayKey",keys(submeta.get("arrayBuffers")));return result;
}
JSValue coercion(JSValue log,const char* label,JSValue value) {
    auto object=JSValue::object();object.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c){push(c[0],c[1]);return c[2];},{log,label,value}));return object;
}
JSValue orderCase() {
    auto result=JSValue::object();
    for(int mode=0;mode<3;++mode) {
        auto log=JSValue::array(),array=make(6);const auto buffer=array.get("buffer");
        getter(array,"buffer",log,buffer);getter(array,"byteOffset",log,mode==0?0:4);
        getter(array,"byteLength",log,mode==0?48:16);getter(array,"constructor",log,ctor(6));
        InterleavedBuffer source(array,3);source.uuid="source";auto meta=JSValue::object();
        if(mode==2) source.toJSON(meta);else source.clone(meta);
        result.set("keyLog:"+std::to_string(mode),log);
    }
    for(int mode=0;mode<4;++mode) {
        auto log=JSValue::array(),input=JSValue::object();getter(input,"length",log,2);getter(input,"0",log,coercion(log,"element0",7));
        getter(input,"1",log,coercion(log,"element1",8));InterleavedBuffer target(nativeArray(Float32Array({1,2,3})),1);
        auto offset=coercion(log,"offset",mode==0?0:mode==1?-1:mode==2?3:std::numeric_limits<double>::infinity());
        auto status=attempt([&]{target.set(input,offset);return JSValue("ok");});
        result.set("set:"+std::to_string(mode),list({status,log,values(target.array)}));
    }
    auto log=JSValue::array();auto source=JSValue::object();getter(source,"length",log,3);getter(source,"0",log,9);
    JSPropertyDescriptor fail;fail.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"1");throw JSTypeError("read failed");},{log});source.defineProperty("1",fail);
    InterleavedBuffer target(nativeArray(Float32Array({1,2,3})),1);auto status=attempt([&]{target.set(source);return JSValue("ok");});
    result.set("partialSet",list({status,log,values(target.array)}));
    return result;
}
JSValue superCase() {
    auto log=JSValue::array(),source=JSValue::object(),result=JSValue::object();source.set("meshPerAttribute",7);
    InstancedInterleavedBuffer value(make(6),3,2);const auto original=InstancedInterleavedBuffer::prototype().getPrototype();
    auto replacement=JSValue::object(original);
    for(const char* name:{"copy","clone","toJSON"}) {
        JSPropertyDescriptor descriptor;descriptor.configurable=true;
        descriptor.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c){
            push(c[0],list({c[1],self.sameValue(c[2])}));
            return JSValue::function([](const JSValue& self,const Args& args,const Args& c){
                push(c[0],list({std::string("call:")+c[1].string(),self.sameValue(c[2]),args[0].get("meshPerAttribute")}));
                auto output=JSValue::object();output.set("custom",c[1]);return output;
            },c);
        },{log,name,value.object()});replacement.defineProperty(name,descriptor);
    }
    InstancedInterleavedBuffer::prototype().setPrototype(replacement);
    value.copy(source);result.set("copyMeshPerAttribute",value.meshPerAttribute.value());
    result.set("clone",value.clone(source));result.set("json",value.toJSON(source));result.set("log",log);
    InstancedInterleavedBuffer::prototype().setPrototype(original);return result;
}
JSValue gcCase() {
    auto root=JSValue::object();
    {auto data=make(6);auto source=InterleavedBuffer(data,3).object();source.set("uuid","source");source.set("self",source);
      auto callback=JSValue::function([](const JSValue&,const Args&,const Args& c){return c[0];},{source});source.set("onUploadCallback",callback);root.set("source",source);}
    root.collectGarbage();auto result=JSValue::object();result.set("callbackCycle",call(root.get("source"),"onUploadCallback").sameValue(root.get("source")));
    root.erase("source");root.collectGarbage();const auto baseline=root.heapObjectCount();
    for(int i=0;i<20;++i){auto array=make(6);auto value=InterleavedBuffer(array,3).object();value.set("self",value);array.set("owner",value);array.get("buffer").set("view",array);}
    root.collectGarbage();result.set("cyclesCollected",root.heapObjectCount()==baseline);
    Float32Array retained({1,2,3,4});
    {auto wrapper=nativeArray(retained);wrapper.set("label","kept");wrapper.get("buffer").set("_uuid","kept-buffer");}
    root.collectGarbage();const auto wrapper=nativeArray(retained);result.set("nativeRetained",list({wrapper.get("label"),wrapper.get("buffer").get("_uuid")}));
    return result;
}
}
int main(){try{
    auto output=JSValue::object();
    for(int kind=0;kind<9;++kind){output.set("base:"+std::to_string(kind),ordinaryCase(kind,false));output.set("instance:"+std::to_string(kind),ordinaryCase(kind,true));output.set("views:"+std::to_string(kind),viewsCase(kind));}
    std::vector<JSValue> strides{0,1,3,2.5,"2",nullptr,JSValue(),-2,true};
    for(std::size_t i=0;i<strides.size();++i)output.set("stride:"+std::to_string(i),countCase(strides[i]));
    output.set("copying",copyingCase());output.set("errors",errorsCase());output.set("callbacks",callbackCase());output.set("bytes",bytesCase());output.set("order",orderCase());output.set("super",superCase());output.set("gc",gcCase());
    std::cout<<output.toJSON()->dump()<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

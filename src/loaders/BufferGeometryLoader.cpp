// Port of three.js BufferGeometryLoader.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/BufferGeometryLoader.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InstancedBufferAttribute.h"
#include "core/InterleavedBufferAttribute.h"
#include <set>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
std::string key(JSValue value){return value.toPrimitive(JSValue::PreferredType::String).toString();}
std::vector<std::string> enumerableKeys(JSValue value) {
    std::vector<std::string> keys;std::set<std::string> visited;
    while(value.isObject()) {
        for(const auto& name:value.ownPropertyNames()) {
            const auto descriptor=value.getOwnPropertyDescriptor(name);
            if(visited.insert(name).second&&descriptor&&descriptor->enumerable.value_or(false))keys.push_back(name);
        }
        value=value.getPrototype();
    }
    return keys;
}
JSValue typedArray(JSValue type,JSValue values) {
    const auto name=key(type);const char* names[]={"Int8Array","Uint8Array","Int16Array","Uint16Array","Int32Array","Uint32Array","Float32Array","Float64Array","Uint8ClampedArray"};
    for(std::size_t i=0;i<std::size(names);++i)if(name==names[i])return NativeArray_detail::constructor(i).construct({values});
    throw JSTypeError("TYPED_ARRAYS[type] is not a constructor");
}
std::shared_ptr<BufferAttribute> ordinaryAttribute(JSValue array,JSValue itemSize,JSValue normalized,bool instanced) {
    const auto size=itemSize.toNumber();if(normalized.isUndefined())normalized=false;
    auto result=std::visit([&](const auto& storage)->std::shared_ptr<BufferAttribute> {
        if constexpr(std::is_same_v<std::decay_t<decltype(storage)>,JSValue>)throw JSTypeError("Expected TypedArray");
        else if(instanced)return std::make_shared<InstancedBufferAttribute>(storage,size,normalized.toBoolean());
        else return std::make_shared<BufferAttribute>(storage,size,normalized.toBoolean());
    },animationArray(array).storage());
    auto wrapper=nativeAttribute(result);wrapper.setHostSlot("rawItemSize",itemSize);wrapper.setHostSlot("rawNormalized",normalized);return result;
}
std::shared_ptr<BufferGeometry> parseGeometryValue(JSValue json) {
    auto interleavedMap=JSValue::object(),arrayBufferMap=JSValue::object();
    const auto getInterleaved=[&](JSValue data,JSValue uuid) {
        if(!interleavedMap.get(key(uuid)).isUndefined())return interleavedMap.get(key(uuid));
        const auto buffers=data.get("interleavedBuffers"),buffer=buffers.get(key(uuid));
        const auto bufferUUID=buffer.get("buffer");JSValue storage;
        if(!arrayBufferMap.get(key(bufferUUID)).isUndefined())storage=arrayBufferMap.get(key(bufferUUID));
        else {
            const auto arrayBuffers=data.get("arrayBuffers"),arrayBuffer=arrayBuffers.get(key(bufferUUID));
            storage=NativeArray_detail::constructor(5).construct({arrayBuffer}).get("buffer");arrayBufferMap.set(key(bufferUUID),storage);
        }
        const auto array=typedArray(buffer.get("type"),storage);InterleavedBuffer interleaved(array,buffer.get("stride"));interleaved.uuid=buffer.get("uuid");
        if(!buffer.get("usage").isUndefined())interleaved.setUsage(buffer.get("usage"));
        interleavedMap.set(key(uuid),interleaved.object());return interleaved.object();
    };
    std::shared_ptr<BufferGeometry> geometry=json.get("isInstancedBufferGeometry").toBoolean()?std::make_shared<InstancedBufferGeometry>():std::make_shared<BufferGeometry>();
    const auto index=json.get("data").get("index");
    if(!index.isUndefined()){const auto type=index.get("type"),array=index.get("array");geometry->setIndex(ordinaryAttribute(typedArray(type,array),1,false,false));}
    const auto readAttribute=[&](JSValue attribute,bool morph)->std::shared_ptr<VertexAttribute> {
        std::shared_ptr<VertexAttribute> result;
        if(attribute.get("isInterleavedBufferAttribute").toBoolean()) {
            const auto data=json.get("data"),uuid=attribute.get("data");const auto interleaved=getInterleaved(data,uuid);
            const auto size=attribute.get("itemSize"),offset=attribute.get("offset"),normalized=attribute.get("normalized");
            result=std::make_shared<InterleavedBufferAttribute>(interleaved,size,offset,normalized);
        } else {
            const auto type=attribute.get("type"),array=attribute.get("array"),typed=typedArray(type,array);
            const auto instanced=!morph&&attribute.get("isInstancedBufferAttribute").toBoolean();
            const auto size=attribute.get("itemSize"),normalized=attribute.get("normalized");result=ordinaryAttribute(typed,size,normalized,instanced);
        }
        auto wrapper=nativeAttribute(result);
        if(!attribute.get("name").isUndefined())wrapper.set("name",attribute.get("name"));
        if(!attribute.get("usage").isUndefined()) {
            const auto usage=attribute.get("usage");
            if(const auto ordinary=std::dynamic_pointer_cast<BufferAttribute>(result))ordinary->setUsage(usage);
            else wrapper.get("setUsage").call(wrapper,{usage});
        }
        if(!attribute.get("gpuType").isUndefined())wrapper.set("gpuType",attribute.get("gpuType"));return result;
    };
    const auto attributes=json.get("data").get("attributes");
    for(const auto& name:enumerableKeys(attributes))geometry->setAttribute(name,readAttribute(attributes.get(name),false));
    const auto morphAttributes=json.get("data").get("morphAttributes");
    if(morphAttributes.toBoolean())for(const auto& name:enumerableKeys(morphAttributes)) {
        const auto attributes=morphAttributes.get(name);std::vector<std::shared_ptr<VertexAttribute>> array;
        const auto length=attributes.get("length");for(double i=0;i<length.toNumber();++i)array.push_back(readAttribute(attributes.get(JSValue(i).toString()),true));
        geometry->morphAttributes[name]=std::move(array);
    }
    if(json.get("data").get("morphTargetsRelative").toBoolean())geometry->morphTargetsRelative=true;
    auto groups=json.get("data").get("groups");if(!groups.toBoolean())groups=json.get("data").get("drawcalls");if(!groups.toBoolean())groups=json.get("data").get("offsets");
    if(!groups.isUndefined()) {
        const auto length=groups.get("length");for(double i=0;!JSValue(i).strictEquals(length);++i) {
            const auto group=groups.get(JSValue(i).toString()),start=group.get("start"),count=group.get("count"),materialIndex=group.get("materialIndex");
            geometry->addGroup(start.toNumber(),count.toNumber(),materialIndex.isUndefined()?0:materialIndex.toNumber());
        }
    }
    const auto sphere=json.get("data").get("boundingSphere");
    if(!sphere.isUndefined()) {
        Sphere value;value.radius=sphere.get("radius").toNumber();const auto center=sphere.get("center");
        const auto x=center.get("0").toNumber(),y=center.get("1").toNumber(),z=center.get("2").toNumber();value.center.set(x,y,z);geometry->boundingSphere=value;
    }
    if(json.get("name").toBoolean())geometry->name=json.get("name").toString();
    if(json.get("userData").toBoolean())geometry->userData=json.get("userData").toJSON().value();return geometry;
}
}
JSValue BufferGeometryLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{initialize(self,arg(args,0));return {};},{},"BufferGeometryLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));auto result=constructor.get("prototype");result.setPrototype(Loader::prototype());
        JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
        descriptor.value=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{return nativeGeometry(parseGeometryValue(arg(args,0)));},{},"parse",1);result.defineProperty("parse",descriptor);
        descriptor.value=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw std::runtime_error("BufferGeometryLoader.load requires the pending native FileLoader transport");},{},"load",4);result.defineProperty("load",descriptor);return result;
    }();return value;
}
BufferGeometryLoader::BufferGeometryLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
std::shared_ptr<BufferGeometry> BufferGeometryLoader::parseGeometry(JSValue json){return std::static_pointer_cast<BufferGeometry>(Loader::parse(json).hostPayload("three.BufferGeometry"));}

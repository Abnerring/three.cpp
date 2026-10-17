// Port of src/loaders/BufferGeometryLoader.js at the pinned revision (see upstream.json).
//
// Plain, interleaved and instanced paths are all implemented. The arrayBuffers table holds uint32
// words keyed by uuid; an interleaved buffer names one of those plus a type and a stride, and every
// attribute naming the same uuid shares one InterleavedBuffer, which the existing test asserts with
// sameValue. instanceCount is deliberately not read: the pinned loader takes the count from the
// geometry data and the test names that behaviour. An interleaved attribute that already carries a
// usage is refused, as upstream refuses a second parse of such a geometry.
//
// Upstream logs to the console when no onError is supplied; there is no console natively, so that
// branch is recorded rather than reproduced.
#include "loaders/BufferGeometryLoader.h"
#include "core/BufferAttribute.h"
#include "core/InstancedBufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/NativeObject.h"
#include "animation/AnimationUtils.h"
#include "loaders/FileLoader.h"
#include "math/Sphere.h"
#include "math/Vector3.h"
#include <cstring>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// The type name selects the class. Array elements are read with a string index: an integer one
// selects the name overload of get and is read as a null pointer.
// itemSizeOverride is used for the index buffer, which upstream always builds with an item size of one
// regardless of what the description says - it usually says nothing.
std::shared_ptr<BufferAttribute> typedAttribute(const JSValue& description,double itemSizeOverride=-1) {
    const auto type=description.get("type").toString();
    const auto values=description.get("array");
    const double length=values.get("length").toNumber();
    const double itemSize=itemSizeOverride>=0?itemSizeOverride:description.get("itemSize").toNumber();
    const auto normalized=description.get("normalized").toBoolean();
    std::vector<double> numbers;
    numbers.reserve(static_cast<std::size_t>(length<0?0:length));
    for(double index=0;index<length;index+=1)
        numbers.push_back(values.get(std::to_string(static_cast<long long>(index))).toNumber());
    // An instanced attribute is a different class, and upstream builds one here; falling through to the
    // plain class would silently drop the per-instance data.
    const auto instanced=description.get("isInstancedBufferAttribute").toBoolean();
    const auto build=[&](auto tag) -> std::shared_ptr<BufferAttribute> {
        using Element=decltype(tag);
        const std::vector<Element> converted(numbers.begin(),numbers.end());
        // Upstream passes only the array, the item size and the normalized flag here, so meshPerAttribute keeps its default even when the document carries one.
        if(instanced) return std::make_shared<InstancedBufferAttribute>(TypedArray<Element>(converted),itemSize,normalized);
        return std::make_shared<TypedBufferAttribute<Element>>(converted,itemSize,normalized);
    };
    if(type=="Float32Array")return build(float{});
    if(type=="Float64Array")return build(double{});
    if(type=="Int8Array")return build(std::int8_t{});
    if(type=="Uint8Array")return build(std::uint8_t{});
    if(type=="Int16Array")return build(std::int16_t{});
    if(type=="Uint16Array")return build(std::uint16_t{});
    if(type=="Int32Array")return build(std::int32_t{});
    if(type=="Uint32Array")return build(std::uint32_t{});
    if(type=="Uint8ClampedArray") {
        // The clamped array is a distinct type in this port: it saturates to 0..255 rather than wrapping,
        // so the elements are assigned through the clamped array itself instead of being cast.
        TypedArray<std::uint8_t, true> values(numbers.size());
        for(std::size_t index=0;index<numbers.size();index+=1) values[index]=numbers[index];
        if(instanced) return std::make_shared<InstancedBufferAttribute>(values,itemSize,normalized);
        return std::make_shared<TypedBufferAttribute<std::uint8_t, true>>(values,itemSize,normalized);
    }
    throw JSTypeError("BufferGeometryLoader: typed array "+type+" is not supported natively");
}
void applyOptionalFields(const JSValue& description,const std::shared_ptr<BufferAttribute>& attribute) {
    const auto name=description.get("name");
    if(!name.isUndefined())attribute->name=name.toString();
    const auto usage=description.get("usage");
    if(!usage.isUndefined())attribute->setUsage(static_cast<int>(usage.toNumber()));
    const auto gpuType=description.get("gpuType");
    if(!gpuType.isUndefined())attribute->gpuType=gpuType;   // upstream keeps the number, not its spelling
}
thread_local JSValue interleavedData;
thread_local std::map<std::string,std::shared_ptr<InterleavedBuffer>> interleavedCache;
std::shared_ptr<InterleavedBuffer> interleavedBufferFor(const std::string& uuid) {
    const auto cached=interleavedCache.find(uuid);
    if(cached!=interleavedCache.end())return cached->second;
    const auto spec=interleavedData.get("interleavedBuffers").get(uuid);
    const auto words=interleavedData.get("arrayBuffers").get(spec.get("buffer").toString());
    const double count=words.get("length").toNumber();
    std::vector<std::uint32_t> storage;
    storage.reserve(static_cast<std::size_t>(count<0?0:count));
    for(double index=0;index<count;index+=1)
        storage.push_back(static_cast<std::uint32_t>(words.get(std::to_string(static_cast<long long>(index))).toNumber()));
    const auto type=spec.get("type").toString();
    std::shared_ptr<InterleavedBuffer> buffer;
    if(type=="Float32Array") {
        // The arrayBuffers table stores uint32 words, so a Float32 view reinterprets those bytes;
        // copying the words into floats would give 1077936128 for 3.0.
        std::vector<float> values(storage.size());
        for(std::size_t index=0;index<storage.size();index+=1) std::memcpy(&values[index],&storage[index],sizeof(float));
        buffer=std::make_shared<InterleavedBuffer>(TypedArray<float>(values),spec.get("stride"));
    } else if(type=="Uint32Array") {
        buffer=std::make_shared<InterleavedBuffer>(TypedArray<std::uint32_t>(storage),spec.get("stride"));
    } else {
        throw JSTypeError("BufferGeometryLoader: interleaved element type "+type+" is not supported natively");
    }
    // The spec carries its own usage, and the test reads it back through the attribute's data.
    if(!spec.get("usage").isUndefined())buffer->setUsage(static_cast<int>(spec.get("usage").toNumber()));
    interleavedCache[uuid]=buffer;
    return buffer;
}
std::shared_ptr<VertexAttribute> attributeFor(const JSValue& description) {
    if(description.get("isInterleavedBufferAttribute").isUndefined()) {
        auto attribute=typedAttribute(description);
        applyOptionalFields(description,attribute);
        return attribute;
    }
    if(!description.get("usage").isUndefined())
        throw JSTypeError("BufferGeometryLoader: an interleaved attribute cannot carry a usage");
    const auto buffer=interleavedBufferFor(description.get("data").toString());
    // Constructed once, with the buffer object as data: an intermediate copy is what made the two
    // attributes point at different objects. The buffer object is the same handle for both of them.
    auto shared=std::make_shared<InterleavedBufferAttribute>(buffer->object(),description.get("itemSize"),description.get("offset"),description.get("normalized"));
    const auto name=description.get("name");
    if(!name.isUndefined())shared->object().set("name",name);
    return std::static_pointer_cast<VertexAttribute>(shared);
}
}
std::shared_ptr<BufferGeometry> BufferGeometryLoader::parseGeometry(JSValue json) {
    const auto data=json.get("data");
    interleavedCache.clear();
    interleavedData=data;
    // Upstream tests the flag for truth, not presence: an explicit false is a plain geometry.
    auto geometry=!json.get("isInstancedBufferGeometry").toBoolean()
        ? std::static_pointer_cast<BufferGeometry>(std::make_shared<BufferGeometry>())
        : std::static_pointer_cast<BufferGeometry>(std::make_shared<InstancedBufferGeometry>());
    const auto index=data.get("index");
    if(!index.isUndefined()) geometry->setIndex(typedAttribute(index, 1.0));
    const auto attributes=data.get("attributes");
    for(const auto& key:attributes.keys()) geometry->setAttribute(key,attributeFor(attributes.get(key)));
    const auto morphAttributes=data.get("morphAttributes");
    if(!morphAttributes.isUndefined()) {
        for(const auto& key:morphAttributes.keys()) {
            const auto list=morphAttributes.get(key);
            const double length=list.get("length").toNumber();
            std::vector<std::shared_ptr<VertexAttribute>> collected;
            for(double index=0;index<length;index+=1)
                collected.push_back(attributeFor(list.get(std::to_string(static_cast<long long>(index)))));
            geometry->morphAttributes[key]=std::move(collected);
        }
    }
    const auto morphTargetsRelative=data.get("morphTargetsRelative");
    if(!morphTargetsRelative.isUndefined()&&morphTargetsRelative.toBoolean()) geometry->morphTargetsRelative=true;
    auto groups=data.get("groups");
    if(groups.isUndefined())groups=data.get("drawcalls");
    if(groups.isUndefined())groups=data.get("offsets");
    if(!groups.isUndefined()) {
        const double length=groups.get("length").toNumber();
        for(double index=0;index<length;index+=1) {
            const auto group=groups.get(std::to_string(static_cast<long long>(index)));
            // A missing materialIndex takes the default of zero, which is what undefined means for a JS
            // parameter; reading it as a number would give NaN instead.
            const auto materialIndex=group.get("materialIndex");
            geometry->addGroup(group.get("start").toNumber(),group.get("count").toNumber(),materialIndex.isUndefined()?0.0:materialIndex.toNumber());
        }
    }
    const auto boundingSphere=data.get("boundingSphere");
    if(!boundingSphere.isUndefined()) {
        // Sphere has no fromJSON here, so the same data is read by hand.
        const auto center=boundingSphere.get("center");
        geometry->boundingSphere=Sphere(Vector3(center.get("0").toNumber(),center.get("1").toNumber(),center.get("2").toNumber()),
            boundingSphere.get("radius").toNumber());
    }
    const auto name=json.get("name");
    if(!name.isUndefined()&&!name.toString().empty())geometry->name=name.toString();   // upstream assigns the field
    const auto userData=json.get("userData");
    if(!userData.isUndefined())geometry->userData=userData.toJSON().value_or(JsonValue::Object{});   // JsonValue, not JSValue
    return geometry;
}
JSValue BufferGeometryLoader::parse(JSValue json){return nativeGeometry(parseGeometry(json));}
BufferGeometryLoader::BufferGeometryLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue BufferGeometryLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"BufferGeometryLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4},{"parse",1}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
                BufferGeometryLoader loader=BufferGeometryLoader::fromObject(receiver);
                if(captures[0].string()=="parse")return loader.parse(arg(args,0));
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
namespace {
struct Pending {
    JSValue onLoad{},onError{},manager{},url{};
};
Pending& pending(){static thread_local Pending value;return value;}
JSValue loadedCallback(const JSValue&,const Args& args,const Args&){
    auto& slot=pending();
    try {
        BufferGeometryLoader parser;
        const auto geometry=parser.parse(Animation_detail::parseJSON(args.empty()?std::string{}:args[0].toString()));
        if(!slot.onLoad.isUndefined())slot.onLoad.call(JSValue{}, {geometry});
    } catch(const std::exception& error) {
        if(!slot.onError.isUndefined())slot.onError.call(JSValue{}, {JSValue(error.what())});
        if(!slot.manager.isUndefined())slot.manager.get("itemError").call(slot.manager, {slot.url});
    }
    return {};
}
}
JSValue BufferGeometryLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto& slot=pending();
    slot.onLoad=onLoad;slot.onError=onError;slot.url=url;
    slot.manager=object_.get("manager");
    FileLoader fileLoader(slot.manager);
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setPath",object_.get("path"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setWithCredentials",object_.get("withCredentials"));
    const auto result=fileLoader.load(url,JSValue::function(loadedCallback,{}, "onLoad", 1),onProgress,onError);
    slot= Pending{};
    return result;
}

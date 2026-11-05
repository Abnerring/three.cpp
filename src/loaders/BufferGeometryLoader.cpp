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
std::shared_ptr<BufferAttribute> typedAttribute(const JSValue& description,double itemSizeOverride=-1,bool honourInstanced=true) {
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
        if(instanced&&honourInstanced) return std::make_shared<InstancedBufferAttribute>(TypedArray<Element>(converted),itemSize,normalized);
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
        if(instanced&&honourInstanced) return std::make_shared<InstancedBufferAttribute>(values,itemSize,normalized);
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
    // Upstream copies the uuid from the document onto the buffer, and the serialised attribute refers to
    // the buffer by that uuid, so leaving the generated one in place changes the output.
    if(!spec.get("uuid").isUndefined())buffer->uuid=spec.get("uuid").toString();
    interleavedCache[uuid]=buffer;
    return buffer;
}
// Morph attributes are always plain buffer attributes upstream, whatever the flag says, hence the
// second parameter: the callers that take attributes from the document pass true.
std::shared_ptr<VertexAttribute> attributeFor(const JSValue& description,bool honourInstanced=true) {
    if(description.get("isInterleavedBufferAttribute").isUndefined()) {
        auto attribute=typedAttribute(description,-1,honourInstanced);
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
    // for (const key in undefined) iterates zero times, so an absent attributes table is not an error.
    if(!attributes.isUndefined())
        for(const auto& key:attributes.keys()) geometry->setAttribute(key,attributeFor(attributes.get(key)));
    const auto morphAttributes=data.get("morphAttributes");
    if(!morphAttributes.isUndefined()) {
        for(const auto& key:morphAttributes.keys()) {
            const auto list=morphAttributes.get(key);
            const double length=list.get("length").toNumber();
            std::vector<std::shared_ptr<VertexAttribute>> collected;
            for(double index=0;index<length;index+=1)
                collected.push_back(attributeFor(list.get(std::to_string(static_cast<long long>(index))), false));
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
#include "loaders/BufferGeometryLoaderSources.inc"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "utils.h"
#include <unordered_set>
#include <limits>
namespace BufferGeometryLoaderManaged {
struct Invocation {
    JSValue receiver,method;
    JSValue operator()(const Args& args={})const{return method.call(receiver,args);}
};
Invocation invoke(const JSValue& receiver,const char* name){return {receiver,receiver.get(name)};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor descriptor;descriptor.value=value;descriptor.writable=descriptor.enumerable=descriptor.configurable=true;object.defineProperty(key,descriptor);}
class ForInCursor {
    JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;
    std::size_t index=0;bool loaded=false;
  public:
    explicit ForInCursor(JSValue value):object(value.isNull()||value.isUndefined()?JSValue(nullptr):value.toObject()){}
    std::optional<JSValue> next(){
        while(!object.isNull()){
            if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}
            while(index<keys.size()){
                const auto key=keys[index++];if(key.isSymbol()||visited.contains(key.string()))continue;
                const auto descriptor=object.getOwnPropertyDescriptor(key);if(!descriptor)continue;
                visited.insert(key.string());if(descriptor->enumerable.value_or(false))return key;
            }
            object=object.getPrototype();loaded=false;
        }
        return {};
    }
};
JSValue instancedGeometryConstructor(const JSValue& base){
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& newTarget,const Args&,const Args& captures)->JSValue{
        auto self=captures[0].construct({},newTarget);
        own(self,"isInstancedBufferGeometry",true);self.set("type","InstancedBufferGeometry");own(self,"instanceCount",std::numeric_limits<double>::infinity());return self;
    },{base},"InstancedBufferGeometry",0);
    constructor.setPrototype(base);constructor.setFunctionSourceText(BufferGeometryLoaderSource::instancedClassSource);
    auto prototype=constructor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor descriptor;descriptor.writable=descriptor.configurable=true;
    auto copy=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{
        const auto parent=captures[0].getPrototype();const auto method=parent.get("copy",self);method.call(self,{arg(args,0)});
        auto target=self;target.set("instanceCount",arg(args,0).get("instanceCount"));return self;
    },{prototype},"copy",1);copy.setFunctionSourceText(BufferGeometryLoaderSource::instancedCopySource);descriptor.value=copy;prototype.defineProperty("copy",descriptor);
    auto toJSON=JSValue::function(+[](const JSValue& self,const Args&,const Args& captures)->JSValue{
        const auto parent=captures[0].getPrototype();const auto method=parent.get("toJSON",self);auto data=method.call(self,{});
        data.set("instanceCount",self.get("instanceCount"));data.set("isInstancedBufferGeometry",true);return data;
    },{prototype},"toJSON",0);toJSON.setFunctionSourceText(BufferGeometryLoaderSource::instancedToJSONSource);descriptor.value=toJSON;prototype.defineProperty("toJSON",descriptor);
    return constructor;
}
JSValue typedArray(const JSValue& imports,const JSValue& type,const JSValue& buffer){
    return imports.get("TYPED_ARRAYS").get(type).construct({buffer});
}
JSValue arrayBuffer(const JSValue& state,const JSValue& json,const JSValue& uuid){
    auto cache=state.get("arrayBufferMap");
    if(!cache.get(uuid).isUndefined())return cache.get(uuid);
    const auto arrays=json.get("arrayBuffers");const auto words=arrays.get(uuid);
    const auto buffer=nativeGlobalBinding("Uint32Array").construct({words}).get("buffer");
    cache.set(uuid,buffer);return buffer;
}
JSValue interleavedBuffer(const JSValue& state,const JSValue& json,const JSValue& uuid){
    auto cache=state.get("interleavedBufferMap");
    if(!cache.get(uuid).isUndefined())return cache.get(uuid);
    const auto buffers=json.get("interleavedBuffers");const auto description=buffers.get(uuid);
    const auto buffer=arrayBuffer(state,json,description.get("buffer"));
    const auto imports=state.get("imports");const auto type=description.get("type");const auto array=typedArray(imports,type,buffer);
    const auto ib=imports.get("InterleavedBuffer").construct({array,description.get("stride")});
    auto result=ib;result.set("uuid",description.get("uuid"));
    if(!description.get("usage").isUndefined())invoke(ib,"setUsage")({description.get("usage")});
    cache.set(uuid,ib);return ib;
}
void optionalAttributeFields(const JSValue& description,JSValue attribute){
    if(!description.get("name").isUndefined())attribute.set("name",description.get("name"));
    if(!description.get("usage").isUndefined())invoke(attribute,"setUsage")({description.get("usage")});
    if(!description.get("gpuType").isUndefined())attribute.set("gpuType",description.get("gpuType"));
}
JSValue makeAttribute(const JSValue& state,const JSValue& json,const JSValue& description,bool morph){
    const auto imports=state.get("imports");JSValue attribute;
    if(description.get("isInterleavedBufferAttribute").toBoolean()){
        // json.data is read anew at this exact source call site. Its getter
        // may return a distinct data object, mutate the document, or reenter.
        const auto data=json.get("data");const auto uuid=description.get("data");
        const auto ib=interleavedBuffer(state,data,uuid);
        attribute=imports.get("InterleavedBufferAttribute").construct({ib,description.get("itemSize"),description.get("offset"),description.get("normalized")});
    }else{
        const auto type=description.get("type");const auto values=description.get("array");const auto array=typedArray(imports,type,values);
        const auto constructor=morph?imports.get("BufferAttribute"):description.get("isInstancedBufferAttribute").toBoolean()?imports.get("InstancedBufferAttribute"):imports.get("BufferAttribute");
        attribute=constructor.construct({array,description.get("itemSize"),description.get("normalized")});
    }
    optionalAttributeFields(description,attribute);return attribute;
}
JSValue parse(const JSValue&,const Args& args,const Args& captures){
    const auto json=arg(args,0),imports=captures[0];
    auto state=JSValue::object(nullptr);own(state,"imports",imports);own(state,"interleavedBufferMap",JSValue::object());own(state,"arrayBufferMap",JSValue::object());
    const auto constructor=json.get("isInstancedBufferGeometry").toBoolean()?imports.get("InstancedBufferGeometry"):imports.get("BufferGeometry");
    auto geometry=constructor.construct();
    const auto index=json.get("data").get("index");
    if(!index.isUndefined()){
        const auto type=index.get("type");const auto values=index.get("array");const auto array=typedArray(imports,type,values);
        invoke(geometry,"setIndex")({imports.get("BufferAttribute").construct({array,1})});
    }
    const auto attributes=json.get("data").get("attributes");
    ForInCursor attributeKeys(attributes);
    while(const auto key=attributeKeys.next()){
        const auto description=attributes.get(*key);const auto attribute=makeAttribute(state,json,description,false);
        invoke(geometry,"setAttribute")({*key,attribute});
    }
    const auto morph=json.get("data").get("morphAttributes");
    if(morph.toBoolean()){
        ForInCursor morphKeys(morph);
        while(const auto key=morphKeys.next()){
            const auto descriptions=morph.get(*key);auto array=JSValue::array();const auto length=descriptions.get("length");
            for(double i=0;i<length.toNumber();++i){
                const auto description=descriptions.get(JSValue(i));const auto attribute=makeAttribute(state,json,description,true);
                invoke(array,"push")({attribute});
            }
            auto destination=geometry.get("morphAttributes");destination.set(*key,array);
        }
    }
    const auto relative=json.get("data").get("morphTargetsRelative");
    if(relative.toBoolean())geometry.set("morphTargetsRelative",true);
    auto groups=json.get("data").get("groups");
    if(!groups.toBoolean())groups=json.get("data").get("drawcalls");
    if(!groups.toBoolean())groups=json.get("data").get("offsets");
    if(!groups.isUndefined()){
        const auto length=groups.get("length");
        for(double i=0;!JSValue(i).strictEquals(length);++i){
            const auto group=groups.get(JSValue(i));invoke(geometry,"addGroup")({group.get("start"),group.get("count"),group.get("materialIndex")});
        }
    }
    const auto sphere=json.get("data").get("boundingSphere");
    if(!sphere.isUndefined())geometry.set("boundingSphere",invoke(imports.get("Sphere").construct(),"fromJSON")({sphere}));
    if(json.get("name").toBoolean())geometry.set("name",json.get("name"));
    if(json.get("userData").toBoolean())geometry.set("userData",json.get("userData"));
    return geometry;
}
JSValue load(const JSValue& self,const Args& args,const Args& captures){
    const auto imports=captures[0],url=arg(args,0),onLoad=arg(args,1),onProgress=arg(args,2),onError=arg(args,3);
    const auto loader=imports.get("FileLoader").construct({self.get("manager")});
    invoke(loader,"setPath")({self.get("path")});invoke(loader,"setRequestHeader")({self.get("requestHeader")});invoke(loader,"setWithCredentials")({self.get("withCredentials")});
    auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        try{
            // Resolve scope.parse before JSON.parse, preserving member getter
            // order and the source catch boundary around the onLoad callback.
            const auto parse=invoke(captures[0],"parse");
            const auto json=invoke(nativeGlobalBinding("JSON"),"parse")({arg(args,0)});
            captures[1].call({}, {parse({json})});
        }catch(...){
            const auto error=nativeExceptionValue(std::current_exception());
            if(captures[2].toBoolean())captures[2].call({}, {error});else ThreeUtils::error({error});
            invoke(captures[0].get("manager"),"itemError")({captures[3]});
        }
        return {};
    },{self,onLoad,onError,url},"",1);
    callback.setFunctionSourceText(BufferGeometryLoaderSource::callback1Source);
    invoke(loader,"load")({url,callback,onProgress,onError});return {};
}
JSValue imports(){
    auto imports=JSValue::object(nullptr);own(imports,"BufferGeometry",nativeBufferGeometryConstructor());
    own(imports,"InstancedBufferGeometry",instancedGeometryConstructor(imports.get("BufferGeometry")));
    own(imports,"BufferAttribute",nativeBufferAttributeConstructor());own(imports,"InstancedBufferAttribute",nativeInstancedBufferAttributeConstructor());
    own(imports,"InterleavedBuffer",InterleavedBuffer::constructor());own(imports,"InterleavedBufferAttribute",InterleavedBufferAttribute::constructor());
    own(imports,"FileLoader",nativeFileLoaderConstructor());own(imports,"Sphere",nativeSphereConstructor());
    auto arrays=JSValue::object();
    for(const auto* name:{"Int8Array","Uint8Array","Uint8ClampedArray","Int16Array","Uint16Array","Int32Array","Uint32Array","Float32Array","Float64Array"})own(arrays,name,nativeGlobalBinding(name));
    own(imports,"TYPED_ARRAYS",arrays);return imports;
}
} // namespace BufferGeometryLoaderManaged

JSValue BufferGeometryLoader::parse(JSValue json){return object_.get("parse").call(object_,{json});}
BufferGeometryLoader::BufferGeometryLoader(JSValue manager):Loader(nativeBufferGeometryLoaderConstructor().construct({manager}),Existing{}){}
JSValue BufferGeometryLoader::prototype(){
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BufferGeometryLoader prototype");
    auto imports=BufferGeometryLoaderManaged::imports();
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& newTarget,const Args& args,const Args& captures)->JSValue{
        return captures[0].construct({arg(args,0)},newTarget);
    },{nativeLoaderConstructor()},"BufferGeometryLoader",1);
    constructor.setPrototype(nativeLoaderConstructor());constructor.setFunctionSourceText(BufferGeometryLoaderSource::classSource);
    auto prototype=constructor.get("prototype");prototype.setPrototype(Loader::prototype());
    JSPropertyDescriptor descriptor;descriptor.writable=descriptor.configurable=true;
    auto load=JSValue::function(BufferGeometryLoaderManaged::load,{imports},"load",4);load.setFunctionSourceText(BufferGeometryLoaderSource::loadSource);descriptor.value=load;prototype.defineProperty("load",descriptor);
    auto parse=JSValue::function(BufferGeometryLoaderManaged::parse,{imports},"parse",1);parse.setFunctionSourceText(BufferGeometryLoaderSource::parseSource);descriptor.value=parse;prototype.defineProperty("parse",descriptor);
    prototype.setInternalSlot("BufferGeometryLoader.constructor",constructor);constructor.retainForHeapLifetime();cached=JSWeakValue(prototype);return prototype;
}
JSValue nativeBufferGeometryLoaderConstructor(){return BufferGeometryLoader::prototype().getInternalSlot("BufferGeometryLoader.constructor");}
JSValue BufferGeometryLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){return object_.get("load").call(object_,{url,onLoad,onProgress,onError});}

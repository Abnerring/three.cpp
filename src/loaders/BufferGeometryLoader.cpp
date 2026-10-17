#include "loaders/BufferGeometryLoader.h"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/NativeObject.h"
#include "animation/AnimationUtils.h"
#include "loaders/FileLoader.h"
#include "math/Sphere.h"
#include "math/Vector3.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// getTypedArray: the type name selects the class. Anything else is a capability this port does not
// have yet, and saying so is better than returning an empty attribute.
std::shared_ptr<BufferAttribute> typedAttribute(const JSValue& description) {
    const auto type=description.get("type").toString();
    const auto values=description.get("array");
    const double length=values.get("length").toNumber();
    const double itemSize=description.get("itemSize").toNumber();
    const auto normalized=description.get("normalized").toBoolean();
    std::vector<double> numbers;
    numbers.reserve(static_cast<std::size_t>(length<0?0:length));
    // Array elements are read with a string index: an integer one selects the name overload of get.
    for(double index=0;index<length;index+=1)numbers.push_back(values.get(std::to_string(static_cast<long long>(index))).toNumber());
    const auto build=[&](auto tag) {
        using Element=decltype(tag);
        std::vector<Element> converted(numbers.begin(),numbers.end());
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
    throw JSTypeError("BufferGeometryLoader: typed array '"+type+"' is not supported natively");
}
void rejectUnsupported(const JSValue& description) {
    if(!description.get("isInterleavedBufferAttribute").isUndefined())
        throw JSTypeError("BufferGeometryLoader: interleaved buffer attributes are not implemented natively yet");
    if(!description.get("isInstancedBufferAttribute").isUndefined())
        throw JSTypeError("BufferGeometryLoader: instanced buffer attributes are not implemented natively yet");
}
void applyOptionalFields(const JSValue& description,const std::shared_ptr<BufferAttribute>& attribute) {
    const auto name=description.get("name");
    if(!name.isUndefined())attribute->name=name.toString();
    const auto usage=description.get("usage");
    if(!usage.isUndefined())attribute->setUsage(static_cast<int>(usage.toNumber()));
    const auto gpuType=description.get("gpuType");
    if(!gpuType.isUndefined())attribute->gpuType=gpuType.toString();
}
}
std::shared_ptr<BufferGeometry> BufferGeometryLoader::parseGeometry(JSValue json) {
    if(!json.get("arrayBuffers").isUndefined())
        throw JSTypeError("BufferGeometryLoader: the arrayBuffers table is not implemented natively yet");
    if(!json.get("isInstancedBufferGeometry").isUndefined())
        throw JSTypeError("BufferGeometryLoader: instanced geometry is not implemented natively yet");
    auto geometry=std::make_shared<BufferGeometry>();
    const auto data=json.get("data");
    const auto index=data.get("index");
    if(!index.isUndefined()) {
        rejectUnsupported(index);
        geometry->setIndex(typedAttribute(index));
    }
    const auto attributes=data.get("attributes");
    for(const auto& key:attributes.keys()) {
        const auto description=attributes.get(key);
        rejectUnsupported(description);
        auto attribute=typedAttribute(description);
        applyOptionalFields(description,attribute);
        geometry->setAttribute(key,attribute);
    }
    const auto morphAttributes=data.get("morphAttributes");
    if(!morphAttributes.isUndefined()) {
        for(const auto& key:morphAttributes.keys()) {
            const auto list=morphAttributes.get(key);
            const double length=list.get("length").toNumber();
            std::vector<std::shared_ptr<VertexAttribute>> collected;
            for(double index=0;index<length;index+=1) {
                const auto description=list.get(std::to_string(static_cast<long long>(index)));
                rejectUnsupported(description);
                auto attribute=typedAttribute(description);
                applyOptionalFields(description,attribute);
                collected.push_back(attribute);
            }
            geometry->morphAttributes[key]=std::move(collected);
        }
    }
    const auto morphTargetsRelative=data.get("morphTargetsRelative");
    if(!morphTargetsRelative.isUndefined()&&morphTargetsRelative.toBoolean())
        geometry->morphTargetsRelative=true;
    auto groups=data.get("groups");
    if(groups.isUndefined())groups=data.get("drawcalls");
    if(groups.isUndefined())groups=data.get("offsets");
    if(!groups.isUndefined()) {
        const double length=groups.get("length").toNumber();
        for(double index=0;index<length;index+=1) {
            const auto group=groups.get(std::to_string(static_cast<long long>(index)));
            geometry->addGroup(group.get("start").toNumber(),group.get("count").toNumber(),group.get("materialIndex").toNumber());
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
    if(!name.isUndefined()&&!name.toString().empty()) {
        auto object=nativeGeometry(geometry);
        object.set("name",name);
    }
    const auto userData=json.get("userData");
    if(!userData.isUndefined()) {
        auto object=nativeGeometry(geometry);
        object.set("userData",userData);
    }
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

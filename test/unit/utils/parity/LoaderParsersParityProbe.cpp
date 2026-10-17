#include "loaders/AnimationLoader.h"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/MaterialLoader.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/InstancedBufferAttribute.h"
#include "textures/Texture.h"
#include "core/JSPromise.h"
#include <cstdio>
#include <iostream>
namespace {
void push(JSValue array,JSValue value){array.get("push").call(array,{value});}
JSValue run(JSValue request) {
    const auto mode=request.get("mode").toString();const auto json=request.get("json");
    if(mode=="loadAsync") {
        using Args=std::vector<JSValue>;Loader loader;auto scope=loader.object(),trace=JSValue::array();const auto variant=request.get("variant");
        scope.set("load",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            push(captures[0],self.sameValue(captures[2])?"load-this":"wrong-this");args[2].call(JSValue{}, {"progress"});
            if(captures[1].strictEquals(0))args[1].call(JSValue{}, {"success"});
            else if(captures[1].strictEquals(1))args[3].call(JSValue{}, {"failure"});
            else if(captures[1].strictEquals(2))throw JSThrownValue("thrown");
            else {auto thenable=JSValue::object();JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {
                push(captures[0],"then-get");return JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{push(captures[0],"then-call");args[0].call(JSValue{}, {"adopted"});return {};},{captures[0]});
            },{captures[0]});thenable.defineProperty("then",descriptor);args[1].call(JSValue{}, {thenable});}
            push(captures[0],"load-end");return {};
        },{trace,variant,scope}));
        const auto promise=loader.loadAsync("url",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue{push(captures[0],self.isUndefined()?args[0]:JSValue("wrong-progress-this"));return {};},{trace}));
        push(trace,"returned");auto fulfill=JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{push(captures[0],args[0]);return {};},{trace});
        promise.get("then").call(promise,{fulfill,fulfill});push(trace,"attached");drainMicrotasks();return trace;
    }
    if(mode=="geometry") {
        BufferGeometryLoader loader;const auto geometry=loader.parseGeometry(json);geometry->uuid="geometry";
        auto fields=JSValue::object();
        for(const auto& [name,attribute]:geometry->attributes) {
            auto item=JSValue::object();const auto buffer=std::dynamic_pointer_cast<BufferAttribute>(attribute);item.set("count",buffer->count);item.set("itemSize",buffer->itemSize);item.set("normalized",buffer->normalized);
            if(const auto interleaved=std::dynamic_pointer_cast<InterleavedBufferAttribute>(attribute)) {
                auto storage=interleaved->array.value().get("buffer");storage.set("_uuid","storage");item.set("interleaved",true);item.set("offset",interleaved->offset.value());
            } else item.set("interleaved",false);
            if(const auto instanced=std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute)){item.set("instanced",true);item.set("meshPerAttribute",instanced->meshPerAttribute);}
            else item.set("instanced",false);
            fields.set(name,item);
        }
        auto serialized=JSValue::fromJSON(geometry->toJSON());
        auto result=JSValue::object();result.set("json",serialized);result.set("attributes",fields);return result;
    }
    if(mode=="animation") {
        AnimationLoader loader;auto result=JSValue::array();for(const auto& clip:loader.parseAnimations(json))push(result,clip->toJSON());return result;
    }
    if(mode=="material") {
        MaterialLoader loader;OrderedMap<std::shared_ptr<Texture>> textures;
        for(const auto& key:request.get("textures").keys()){auto texture=std::make_shared<Texture>();texture->uuid=key;texture->source->uuid="source";textures[key]=texture;}loader.setTextures(textures);
        const auto material=loader.parseMaterial(json);auto result=JSValue::object();result.set("type",material->type);result.set("json",JSValue::fromJSON(material->toJSON()));return result;
    }
    throw std::runtime_error("Unknown parser case");
}
}
int main() {
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto output=JSValue::array();
    for(std::uint32_t i=0;i<requests.length();++i)try{push(output,run(requests.get(std::to_string(i))));}catch(const JSTypeError&){push(output,"TypeError");}catch(const std::exception&){push(output,"Error");}
    std::cout<<output.toJSON()->dump();
}

#include "loaders/CubeTextureLoader.h"
#include "core/NativeObject.h"
#include "loaders/ImageLoader.h"
#include "textures/CubeTexture.h"
#include <memory>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// The image callback carries no state of its own, so the texture, its faces, the slot being filled,
// the running count and the caller's callbacks live in a thread local. The injected decoder answers
// synchronously, so nothing is in flight between calls.
struct Pending {
    JSValue texture{},onLoad{},onError{};
    std::shared_ptr<CubeTexture> cube{};
    TextureImage::Ptr faces{};
    double loaded=0;
    double slot=0;
};
Pending& pending(){static thread_local Pending value;return value;}
}
CubeTextureLoader::CubeTextureLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue CubeTextureLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"CubeTextureLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
                CubeTextureLoader loader=CubeTextureLoader::fromObject(receiver);
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
JSValue CubeTextureLoader::load(JSValue urls,JSValue onLoad,JSValue onProgress,JSValue onError){
    static_cast<void>(onProgress);
    auto texture=std::make_shared<CubeTexture>();
    auto textureObject=nativeTexture(texture);
    textureObject.set("colorSpace","srgb");
    auto faces=TextureImage::array();
    texture->images=faces;
    auto& slot=pending();
    slot.texture=textureObject;
    slot.cube=texture;
    slot.faces=faces;
    slot.onLoad=onLoad;
    slot.onError=onError;
    slot.loaded=0;
    ImageLoader imageLoader(object_.get("manager"));
    const auto call=[&](const char* name,const JSValue& value){const auto method=imageLoader.object().get(name);if(!method.isUndefined())method.call(imageLoader.object(),{value});};
    call("setCrossOrigin",object_.get("crossOrigin"));
    call("setPath",object_.get("path"));
    const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{
        auto& entry=pending();
        if(!args.empty()&&entry.faces) {
            const auto image=args[0];
            auto face=std::make_shared<TextureImage>(image.get("width").toNumber(),image.get("height").toNumber());
            const auto data=image.get("data");
            if(!data.isUndefined())face->data=data;
            const auto index=static_cast<std::size_t>(entry.slot<0?0:entry.slot);
            if(entry.faces->faces.size()<=index)entry.faces->faces.resize(index+1);
            entry.faces->faces[index]=face;
        }
        entry.loaded+=1;
        if(entry.loaded==6) {
            entry.texture.set("needsUpdate",true);
            if(!entry.onLoad.isUndefined())entry.onLoad.call(JSValue{}, {entry.texture});
        }
        return {};
    },{},"onImage",1);
    const double length=urls.get("length").toNumber();
    for(double index=0;index<length;index+=1) {
        slot.slot=index;
        imageLoader.load(urls.get(std::to_string(static_cast<long long>(index))),callback,JSValue{},slot.onError);
    }
    slot=Pending{};
    return textureObject;
}
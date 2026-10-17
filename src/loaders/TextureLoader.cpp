#include "loaders/TextureLoader.h"
#include "core/NativeObject.h"
#include "loaders/ImageLoader.h"
#include "textures/Texture.h"
#include <memory>
#include <string>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
}
TextureLoader::TextureLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue TextureLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"TextureLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
                TextureLoader loader=TextureLoader::fromObject(receiver);
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
JSValue TextureLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto texture=std::make_shared<Texture>();
    auto textureObject=nativeTexture(texture);
    ImageLoader imageLoader(object_.get("manager"));
    const auto call=[&](const char* name,const JSValue& value){const auto method=imageLoader.object().get(name);if(!method.isUndefined())method.call(imageLoader.object(),{value});};
    call("setCrossOrigin",object_.get("crossOrigin"));
    call("setPath",object_.get("path"));
    // The callback carries no state of its own, so the texture it fills is parked in a thread local
    // slot; the injected decoder answers synchronously, so nothing is in flight across calls.
    struct Pending { JSValue texture{},onLoad{}; };
    static thread_local Pending* volatile current=nullptr;
    static thread_local Pending slot;
    slot.texture=textureObject;
    slot.onLoad=onLoad;
    current=&slot;
    const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{
        auto& entry=slot;
        const auto image=args.empty()?JSValue{}:args[0];
        entry.texture.set("image",image);
        entry.texture.set("needsUpdate",true);
        if(!entry.onLoad.isUndefined())entry.onLoad.call(JSValue{}, {entry.texture});
        return {};
    },{},"onImage",1);
    imageLoader.load(url,callback,onProgress,onError);
    current=nullptr;
    return textureObject;
}

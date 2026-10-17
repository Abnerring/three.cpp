#include "loaders/CompressedTextureLoader.h"
#include "constants.h"
#include "core/NativeObject.h"
#include "loaders/FileLoader.h"
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// The file callback carries no state of its own, so the texture being filled and the caller's callbacks
// are parked in thread locals; FileLoader answers synchronously in this port.
std::shared_ptr<CompressedTexture>& pendingTexture(){static thread_local std::shared_ptr<CompressedTexture> value;return value;}
JSValue& pendingTextureObject(){static thread_local JSValue value;return value;}
JSValue& pendingOnLoad(){static thread_local JSValue value;return value;}
// The object whose parse must run: upstream closes over the loader instance, so the native equivalent is
// the instance itself, which is what makes a subclass override take part.
CompressedTextureLoader*& selfLoader(){static thread_local CompressedTextureLoader* value=nullptr;return value;}
}
CompressedTextureLoader::CompressedTextureLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue CompressedTextureLoader::parse(JSValue buffer,JSValue premultiplyAlpha) {
    static_cast<void>(buffer);
    static_cast<void>(premultiplyAlpha);
    throw JSTypeError("CompressedTextureLoader: parse is abstract; a subclass provides it");
}
void CompressedTextureLoader::applyTexData(const std::shared_ptr<CompressedTexture>& texture,JSValue texDatas) {
    const auto defined=[&](const char* name){return !texDatas.get(name).isUndefined();};
    auto image=texture->image.get();
    if(!image) { image=std::make_shared<TextureImage>(); texture->image=image; }
    image->width=texDatas.get("width").toNumber();
    image->height=texDatas.get("height").toNumber();
    if(defined("mipmaps")) {
        const auto mipmaps=texDatas.get("mipmaps");
        const double count=mipmaps.get("length").toNumber();
        auto chain=std::make_shared<std::vector<Texture::Image>>();
        chain->reserve(static_cast<std::size_t>(count<0?0:count));
        for(double index=0;index<count;index+=1) {
            const auto level=mipmaps.get(std::to_string(static_cast<long long>(index)));
            chain->push_back(std::make_shared<TextureImage>(level.get("data"),level.get("width").toNumber(),level.get("height").toNumber()));
        }
        texture->mipmaps=chain;
    }
    if(texDatas.get("mipmapCount").toNumber()==1)texture->minFilter=LinearFilter;
    if(defined("format"))texture->format=texDatas.get("format").toNumber();
    texture->version+=1;
}
JSValue CompressedTextureLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    // A string also has a length, so the six-url array form is recognised by the value being an object.
    if(url.type()==JSValue::Type::Object&&!url.get("length").isUndefined())
        throw JSTypeError("CompressedTextureLoader: the six-url array form is not implemented natively");
    auto texture=std::make_shared<CompressedTexture>();
    pendingTexture()=texture;
    pendingTextureObject()=nativeTexture(texture);
    pendingOnLoad()=onLoad;
    selfLoader()=this;
    FileLoader fileLoader(object_.get("manager"));
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setPath",object_.get("path"));
    call("setResponseType",JSValue("arraybuffer"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setWithCredentials",object_.get("withCredentials"));
    const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{
        const auto texDatas=selfLoader()->parse(args.empty()?JSValue{}:args[0], JSValue(true));
        if(!texDatas.get("isCubemap").isUndefined()&&texDatas.get("isCubemap").toBoolean())
            throw JSTypeError("CompressedTextureLoader: a cubemap packed into one file is not implemented natively");
        CompressedTextureLoader loader;
        loader.applyTexData(pendingTexture(), texDatas);
        if(!pendingOnLoad().isUndefined())pendingOnLoad().call(JSValue{}, {pendingTextureObject()});
        return {};
    },{},"onLoad",1);
    const auto result=fileLoader.load(url,callback,onProgress,onError);
    static_cast<void>(result);
    pendingTexture().reset();
    pendingOnLoad()=JSValue{};
    selfLoader()=nullptr;
    return pendingTextureObject();
}
JSValue CompressedTextureLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"CompressedTextureLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
                CompressedTextureLoader loader=CompressedTextureLoader::fromObject(receiver);
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}

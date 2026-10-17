#include "loaders/DataTextureLoader.h"
#include "core/NativeObject.h"
#include "loaders/FileLoader.h"
#include "constants.h"
#include <memory>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// The file callback carries no state of its own, so the texture being filled and the caller's callbacks
// are parked in thread locals. FileLoader answers synchronously in this port, so nothing is in flight.
// The instance whose parse must run: upstream closes over the loader, so the native equivalent is the
// instance itself.
DataTextureLoader*& selfLoader(){static thread_local DataTextureLoader* value=nullptr;return value;}
std::shared_ptr<DataTexture>& pendingTexture(){static thread_local std::shared_ptr<DataTexture> value;return value;}
JSValue& pendingTextureObject(){static thread_local JSValue value;return value;}
JSValue& pendingOnLoad(){static thread_local JSValue value;return value;}
JSValue& pendingOnError(){static thread_local JSValue value;return value;}
}
DataTextureLoader::DataTextureLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue DataTextureLoader::parse(JSValue buffer) {
    static_cast<void>(buffer);
    throw JSTypeError("DataTextureLoader: parse is abstract; a subclass provides it");
}
void DataTextureLoader::applyTexData(const std::shared_ptr<DataTexture>& texture,JSValue texData) {
    const auto defined=[&](const char* name){return !texData.get(name).isUndefined();};
    auto image=texture->image.get();
    if(!image) { image=std::make_shared<TextureImage>(); texture->image=image; }
    if(defined("image")) {
        // A record form of an image: the same three fields, read through the same names.
        const auto source=texData.get("image");
        image->width=source.get("width").toNumber();
        image->height=source.get("height").toNumber();
        const auto data=source.get("data");
        if(!data.isUndefined())image->data=data;
    } else if(defined("data")) {
        image->width=texData.get("width").toNumber();
        image->height=texData.get("height").toNumber();
        image->data=texData.get("data");
    }
    texture->wrapS=defined("wrapS")?texData.get("wrapS").toNumber():ClampToEdgeWrapping;
    texture->wrapT=defined("wrapT")?texData.get("wrapT").toNumber():ClampToEdgeWrapping;
    texture->magFilter=defined("magFilter")?texData.get("magFilter").toNumber():LinearFilter;
    texture->minFilter=defined("minFilter")?texData.get("minFilter").toNumber():LinearFilter;
    texture->anisotropy=defined("anisotropy")?texData.get("anisotropy").toNumber():1;
    if(defined("colorSpace"))texture->colorSpace=texData.get("colorSpace").toString();
    if(defined("flipY"))texture->flipY=texData.get("flipY").toBoolean();
    if(defined("format"))texture->format=texData.get("format").toNumber();
    if(defined("type"))texture->type=texData.get("type").toNumber();
    if(defined("mipmaps"))
        throw JSTypeError("DataTextureLoader: assigning mipmaps is not implemented natively");
    if(texData.get("mipmapCount").toNumber()==1)texture->minFilter=LinearFilter;
    if(defined("generateMipmaps"))texture->generateMipmaps=texData.get("generateMipmaps").toBoolean();
    texture->version+=1;
}
JSValue DataTextureLoader::createDataTexture(JSValue buffer) {
    auto texture=std::make_shared<DataTexture>();
    applyTexData(texture,parse(buffer));
    return nativeTexture(texture);
}
JSValue DataTextureLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto texture=std::make_shared<DataTexture>();
    pendingTexture()=texture;
    pendingTextureObject()=nativeTexture(texture);
    pendingOnLoad()=onLoad;
    pendingOnError()=onError;
    selfLoader()=this;
    FileLoader fileLoader(object_.get("manager"));
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setResponseType",JSValue("arraybuffer"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setPath",object_.get("path"));
    call("setWithCredentials",object_.get("withCredentials"));
    const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{
        JSValue texData{};
        try {
            // Dispatched through the object, so a subclass that overrides parse takes part exactly as it
            // does upstream, where the callback closes over the loader instance.
            texData=selfLoader()->parse(args.empty()?JSValue{}:args[0]);
        } catch(const std::exception& error) {
            // Upstream logs to the console when no onError was supplied; there is no console here, so
            // that branch is recorded rather than reproduced. Either way the texture is left unapplied.
            if(!pendingOnError().isUndefined())pendingOnError().call(JSValue{}, {JSValue(error.what())});
            return {};
        }
        DataTextureLoader loader;
        loader.applyTexData(pendingTexture(), texData);
        if(!pendingOnLoad().isUndefined())pendingOnLoad().call(JSValue{}, {pendingTextureObject(), texData});
        return {};
    },{},"onLoad",1);
    const auto result=fileLoader.load(url,callback,onProgress,onError);
    static_cast<void>(result);
    pendingTexture().reset();
    pendingOnLoad()=JSValue{};
    pendingOnError()=JSValue{};
    return pendingTextureObject();
}
JSValue DataTextureLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"DataTextureLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4},{"createDataTexture",1}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
                DataTextureLoader loader=DataTextureLoader::fromObject(receiver);
                if(captures[0].string()=="load")return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
                return loader.createDataTexture(arg(args,0));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
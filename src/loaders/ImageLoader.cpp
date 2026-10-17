#include "loaders/ImageLoader.h"
#include "loaders/Cache.h"
#include "loaders/LoadingManager.h"
#include <map>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue& decoderSlot(){static thread_local JSValue value;return value;}
// One entry per url still being decoded, holding the callbacks that arrived while it was in flight.
// Upstream keys this by the image object; every url has exactly one image here, so the url is the
// same key.
struct Waiting { JSValue onLoad{},onError{}; };
std::map<std::string,std::vector<Waiting>>& pendingMap(){static thread_local std::map<std::string,std::vector<Waiting>> value;return value;}
}
void ImageLoader::setDecoder(JSValue value){decoderSlot()=std::move(value);}
JSValue ImageLoader::decoder(){return decoderSlot();}
ImageLoader::ImageLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue ImageLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"ImageLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
                ImageLoader loader=ImageLoader::fromObject(receiver);
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
JSValue ImageLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    static_cast<void>(onProgress);   // unsupported upstream, and not consulted here
    JSValue self=object_;
    const auto path=self.get("path");
    if(!path.isUndefined())url=JSValue(path.toString()+url.toString());
    auto manager=LoadingManager::fromObject(self.get("manager"));
    url=manager.resolveURL(url);
    const std::string urlText=url.toString();
    const JSValue key("image:"+urlText);
    const auto cached=Cache.get(key);
    if(!cached.isUndefined()) {
        // A complete record calls back at once; an incomplete one queues this pair. Upstream schedules
        // the callback with a zero timeout, which an injected decoder makes unnecessary.
        if(cached.get("complete").toBoolean()) {
            manager.itemStart(url);
            if(!onLoad.isUndefined())onLoad.call(self,{cached});
            manager.itemEnd(url);
        } else {
            pendingMap()[urlText].push_back({onLoad,onError});
        }
        return cached;
    }
    const auto decoder=decoderSlot();
    if(decoder.isUndefined())
        throw JSTypeError("THREE.ImageLoader: no decoder is configured; call ImageLoader::setDecoder before loading "+urlText);
    auto request=JSValue::object();
    request.set("url",url);
    const auto crossOrigin=self.get("crossOrigin");
    if(!crossOrigin.isUndefined()&&urlText.rfind("data:",0)!=0)request.set("crossOrigin",crossOrigin);
    manager.itemStart(url);
    JSValue created;
    try {
        created=decoder.call(self,{request});
        created.set("complete",true);
    } catch(const std::exception& error) {
        Cache.remove(key);
        const auto waiting=pendingMap().find(urlText);
        if(waiting!=pendingMap().end()) {
            for(const auto& callback:waiting->second)
                if(!callback.onError.isUndefined())callback.onError.call(self,{JSValue(error.what())});
            pendingMap().erase(waiting);
        }
        manager.itemError(url);
        manager.itemEnd(url);
        if(!onError.isUndefined())onError.call(self,{JSValue(error.what())});
        throw;
    }
    Cache.add(key,created);
    if(!onLoad.isUndefined())onLoad.call(self,{created});
    const auto waiting=pendingMap().find(urlText);
    if(waiting!=pendingMap().end()) {
        for(const auto& callback:waiting->second)
            if(!callback.onLoad.isUndefined())callback.onLoad.call(self,{created});
        pendingMap().erase(waiting);
    }
    manager.itemEnd(url);
    return created;
}

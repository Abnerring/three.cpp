#include "loaders/ImageBitmapLoader.h"

#include "loaders/Cache.h"
#include "loaders/LoadingManager.h"
#include <stdexcept>

namespace {
JSValue& decoderSlot() {
    static thread_local JSValue value;
    return value;
}
}

ImageBitmapLoader::ImageBitmapLoader(JSValue manager):Loader(std::move(manager)) {
    object_.set("isImageBitmapLoader", true);
    // Upstream starts from a single option and replaces the record wholesale in setOptions.
    auto options = JSValue::object();
    options.set("premultiplyAlpha", "none");
    object_.set("options", options);
}

void ImageBitmapLoader::setDecoder(JSValue decoder) { decoderSlot() = std::move(decoder); }
JSValue ImageBitmapLoader::decoder() { return decoderSlot(); }

JSValue ImageBitmapLoader::setOptions(JSValue options) {
    object_.set("options", options);
    return object_;
}

JSValue ImageBitmapLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    static_cast<void>(onProgress);   // unsupported upstream, and not consulted here either
    JSValue self=object_;
    const auto path=self.get("path");
    if(!path.isUndefined())url=JSValue(path.toString()+url.toString());
    auto manager=LoadingManager::fromObject(self.get("manager"));
    url=manager.resolveURL(url);
    const std::string urlText=url.toString();
    const JSValue key("image-bitmap:"+urlText);
    const auto cached=Cache.get(key);
    if(!cached.isUndefined()) {
        manager.itemStart(url);
        if(!onLoad.isUndefined())onLoad.call(self,{cached});
        manager.itemEnd(url);
        return cached;
    }
    const auto decoder=decoderSlot();
    if(decoder.isUndefined())
        throw JSTypeError("THREE.ImageBitmapLoader: no decoder is configured; call ImageBitmapLoader::setDecoder before loading "+urlText);
    auto request=JSValue::object();
    request.set("url",url);
    request.set("options",self.get("options"));
    const auto crossOrigin=self.get("crossOrigin");
    if(!crossOrigin.isUndefined()&&urlText.rfind("data:",0)!=0)request.set("crossOrigin",crossOrigin);
    manager.itemStart(url);
    JSValue created;
    try {
        created=decoder.call(self,{request});
    } catch(const std::exception& error) {
        Cache.remove(key);
        manager.itemError(url);
        manager.itemEnd(url);
        if(!onError.isUndefined())onError.call(self,{JSValue(error.what())});
        throw;
    }
    Cache.add(key,created);
    if(!onLoad.isUndefined())onLoad.call(self,{created});
    manager.itemEnd(url);
    return created;
}
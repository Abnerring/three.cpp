// Port of three.js LoadingManager.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/LoadingManager.h"
#include "core/AbortController.h"
#include "core/UnicodeNormalization.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver,state=captures[1];const auto scope=captures[0];const auto& name=captures[2].string();
    const auto url=arg(args,0);
    if(name=="itemStart") {
        state.set("itemsTotal",state.get("itemsTotal").toNumber()+1);
        if(state.get("isLoading").strictEquals(false)&&!scope.get("onStart").isUndefined())
            scope.get("onStart").call(scope,{url,state.get("itemsLoaded"),state.get("itemsTotal")});
        state.set("isLoading",true);return {};
    }
    if(name=="itemEnd") {
        state.set("itemsLoaded",state.get("itemsLoaded").toNumber()+1);
        if(!scope.get("onProgress").isUndefined())scope.get("onProgress").call(scope,{url,state.get("itemsLoaded"),state.get("itemsTotal")});
        if(state.get("itemsLoaded").strictEquals(state.get("itemsTotal"))) {
            state.set("isLoading",false);if(!scope.get("onLoad").isUndefined())scope.get("onLoad").call(scope);
        }
        return {};
    }
    if(name=="itemError"){if(!scope.get("onError").isUndefined())scope.get("onError").call(scope,{url});return {};}
    if(name=="resolveURL") {
        const auto normalized=url.type()==JSValue::Type::String?JSValue(normalizeUnicodeNFC(url.string())):url.get("normalize").call(url,{"NFC"});
        const auto modifier=state.get("urlModifier");return modifier.toBoolean()?modifier.call(JSValue{}, {normalized}):normalized;
    }
    if(name=="setURLModifier"){state.set("urlModifier",url);return self;}
    auto handlers=state.get("handlers");
    if(name=="addHandler"){handlers.get("push").call(handlers,{url,arg(args,1)});return self;}
    if(name=="removeHandler") {
        const auto index=handlers.get("indexOf").call(handlers,{url});
        if(!index.strictEquals(-1))handlers.get("splice").call(handlers,{index,2});return self;
    }
    if(name=="getHandler") {
        for(std::uint32_t i=0,length=handlers.length();i<length;i+=2) {
            auto regex=handlers.get(std::to_string(i));const auto loader=handlers.get(std::to_string(i+1));
            if(regex.get("global").toBoolean())regex.set("lastIndex",0);
            if(regex.get("test").call(regex,{url}).toBoolean())return loader;
        }
        return nullptr;
    }
    if(name=="abort") {
        const auto controller=self.get("abortController");controller.get("abort").call(controller);
        self.set("_abortController",nullptr);return self;
    }
    throw JSTypeError("Unknown LoadingManager method");
}
}
void LoadingManager::initialize(JSValue self,JSValue onLoad,JSValue onProgress,JSValue onError) {
    self.set("onStart",{});self.set("onLoad",onLoad);self.set("onProgress",onProgress);self.set("onError",onError);self.set("_abortController",nullptr);
    auto state=JSValue::object();state.set("isLoading",false);state.set("itemsLoaded",0);state.set("itemsTotal",0);state.set("urlModifier",{});state.set("handlers",JSValue::array());
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"itemStart",1},{"itemEnd",1},{"itemError",1},{"resolveURL",1},{"setURLModifier",1},{"addHandler",2},{"removeHandler",1},{"getHandler",1},{"abort",0}})
        self.set(name,JSValue::function(method,{self,state,name},name,arity));
}
JSValue LoadingManager::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{initialize(self,arg(args,0),arg(args,1),arg(args,2));return {};},{},"LoadingManager",3);
        auto prototype=constructor.get("prototype");JSPropertyDescriptor descriptor;descriptor.configurable=true;
        descriptor.getter=JSValue::function([](const JSValue& receiver,const Args&,const Args&)->JSValue {
            auto self=receiver;if(!self.get("_abortController").toBoolean())self.set("_abortController",createNativeAbortController());return self.get("_abortController");
        },{},"get abortController");prototype.defineProperty("abortController",descriptor);return prototype;
    }();return value;
}
LoadingManager::LoadingManager(JSValue onLoad,JSValue onProgress,JSValue onError):object_(JSValue::object(prototype())){initialize(object_,onLoad,onProgress,onError);}
void LoadingManager::itemStart(JSValue url){object_.get("itemStart").call(object_,{url});}
void LoadingManager::itemEnd(JSValue url){object_.get("itemEnd").call(object_,{url});}
void LoadingManager::itemError(JSValue url){object_.get("itemError").call(object_,{url});}
JSValue LoadingManager::resolveURL(JSValue url){return object_.get("resolveURL").call(object_,{url});}
LoadingManager& LoadingManager::setURLModifier(JSValue value){object_.get("setURLModifier").call(object_,{value});return *this;}
LoadingManager& LoadingManager::addHandler(JSValue regex,JSValue loader){object_.get("addHandler").call(object_,{regex,loader});return *this;}
LoadingManager& LoadingManager::removeHandler(JSValue regex){object_.get("removeHandler").call(object_,{regex});return *this;}
JSValue LoadingManager::getHandler(JSValue file){return object_.get("getHandler").call(object_,{file});}
LoadingManager& LoadingManager::abort(){object_.get("abort").call(object_);return *this;}
thread_local LoadingManager DefaultLoadingManager;

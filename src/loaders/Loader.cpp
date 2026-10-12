// Port of three.js Loader.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/Loader.h"
#include "core/JSPromise.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver;const auto& name=captures[0].string();
    if(name=="load"||name=="parse")return {};
    if(name=="abort")return self;
    if(name=="loadAsync") {
        return createNativePromise(JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue {
            const auto scope=captures[0];scope.get("load").call(scope,{captures[1],arg(args,0),captures[2],arg(args,1)});return {};
        },{self,arg(args,0),arg(args,1)}));
    }
    const char* property=name=="setCrossOrigin"?"crossOrigin":name=="setWithCredentials"?"withCredentials":name=="setPath"?"path":name=="setResourcePath"?"resourcePath":"requestHeader";
    self.set(property,arg(args,0));return self;
}
}
void Loader::initialize(JSValue self,JSValue manager) {
    self.set("manager",manager.isUndefined()?DefaultLoadingManager.object():manager);self.set("crossOrigin","anonymous");self.set("withCredentials",false);
    self.set("path","");self.set("resourcePath","");self.set("requestHeader",JSValue::object());
}
JSValue Loader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{initialize(self,arg(args,0));return {};},{},"Loader",1);
        constructor.set("DEFAULT_MATERIAL_NAME",DEFAULT_MATERIAL_NAME);auto result=constructor.get("prototype");
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",0},{"parse",0},{"loadAsync",2},{"setCrossOrigin",1},{"setWithCredentials",1},{"setPath",1},{"setResourcePath",1},{"setRequestHeader",1},{"abort",0}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;descriptor.value=JSValue::function(method,{name},name,arity);result.defineProperty(name,descriptor);
        }
        return result;
    }();return value;
}
Loader::Loader(JSValue manager):object_(JSValue::object(prototype())){initialize(object_,manager);}
JSValue Loader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){return object_.get("load").call(object_,{url,onLoad,onProgress,onError});}
JSValue Loader::loadAsync(JSValue url,JSValue onProgress){return object_.get("loadAsync").call(object_,{url,onProgress});}
JSValue Loader::parse(JSValue data){return object_.get("parse").call(object_,{data});}
Loader& Loader::setCrossOrigin(JSValue value){object_.get("setCrossOrigin").call(object_,{value});return *this;}
Loader& Loader::setWithCredentials(JSValue value){object_.get("setWithCredentials").call(object_,{value});return *this;}
Loader& Loader::setPath(JSValue value){object_.get("setPath").call(object_,{value});return *this;}
Loader& Loader::setResourcePath(JSValue value){object_.get("setResourcePath").call(object_,{value});return *this;}
Loader& Loader::setRequestHeader(JSValue value){object_.get("setRequestHeader").call(object_,{value});return *this;}
Loader& Loader::abort(){object_.get("abort").call(object_);return *this;}

#include "loaders/FileLoader.h"
#include "loaders/Cache.h"
#include "loaders/LoadingManager.h"
#include "core/JSValue.h"
#include <cctype>
#include <string>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue& transportSlot(){static thread_local JSValue value;return value;}
JSValue& loadingMap(){static thread_local JSValue value=JSValue::object();return value;}
std::string header(const JSValue& response,const char* name){
    const auto headers=response.get("headers");
    if(headers.isUndefined())return {};
    const auto getter=headers.get("get");
    if(getter.isUndefined())return {};
    const auto value=getter.call(headers,{JSValue(name)});
    return value.isUndefined()?std::string{}:value.toString();
}
JSValue responseBody(const JSValue& response,const std::string& responseType,const std::string& mimeType){
    const auto body=response.get("body");
    const std::string text=body.isUndefined()?std::string{}:body.toString();
    if(responseType=="arraybuffer")return JSValue(text);
    if(responseType=="json")return JSValue::fromJSON(text);
    if(responseType=="blob"||responseType=="document")
        throw JSTypeError(std::string("THREE.FileLoader: responseType '")+responseType+"' is not supported natively; use "+FileLoader::RESPONSE_TYPES_SUPPORTED);
    if(mimeType.empty())return JSValue(text);
    const auto charset=mimeType.find("charset=");
    if(charset!=std::string::npos){
        std::string label=mimeType.substr(charset+8);
        while(!label.empty()&&(label.back()=='"'||label.back()==';'||label.back()==' '))label.pop_back();
        while(!label.empty()&&label.front()=='"')label.erase(label.begin());
        for(auto& ch:label)ch=char(std::tolower(static_cast<unsigned char>(ch)));
        if(!label.empty()&&label!="utf-8"&&label!="utf8")
            throw JSTypeError("THREE.FileLoader: only utf-8 decoding is available natively, got '"+label+"'");
    }
    return JSValue(text);
}
JSValue fileLoad(const JSValue& selfValue,const Args& args){
    JSValue self=selfValue;
    JSValue url=arg(args,0);
    if(url.isUndefined())url=JSValue("");
    const auto path=self.get("path");
    if(!path.isUndefined())url=JSValue(path.toString()+url.toString());
    auto manager=LoadingManager::fromObject(self.get("manager"));
    url=manager.resolveURL(url);
    const std::string urlText=url.toString();
    const JSValue key(std::string("file:")+urlText);
    const auto cached=Cache.get(key);
    if(!cached.isUndefined()){
        manager.itemStart(url);
        const auto onLoad=arg(args,1);
        if(!onLoad.isUndefined())onLoad.call(self,{cached});
        manager.itemEnd(url);
        return {};
    }
    auto& loading=loadingMap();
    if(!loading.get(urlText).isUndefined())return {};
    loading.set(urlText,JSValue::array());
    const auto transport=transportSlot();
    if(transport.isUndefined()){
        manager.itemError(url);
        throw JSTypeError("THREE.FileLoader: no transport is configured; call FileLoader::setTransport before loading "+urlText);
    }
    auto request=JSValue::object();
    request.set("url",url);
    request.set("method","GET");
    request.set("headers",self.get("requestHeader"));
    request.set("credentials",self.get("withCredentials").toBoolean()?JSValue("include"):JSValue("same-origin"));
    auto signal=JSValue::object();
    signal.set("aborted",false);
    request.set("signal",signal);
    manager.itemStart(url);
    const auto mimeType=self.get("mimeType").toString();
    const auto responseType=self.get("responseType").toString();
    const auto response=transport.call(self,{request});
    const double status=response.get("status").toNumber();
    if(status!=200&&status!=0){
        loading.set(urlText,JSValue{});
        manager.itemError(url);
        throw JSTypeError("fetch for \""+urlText+"\" responded with "+std::to_string(int(status))+": "+response.get("statusText").toString());
    }
    // A status of 0 is treated as success like upstream; the upstream console warning for it is
    // deliberately not reproduced because no console exists natively (see the header note).
    const auto onProgress=arg(args,2);
    if(!onProgress.isUndefined()){
        const std::string sizeHeader=header(response,"X-File-Size");
        const std::string lengthHeader=header(response,"Content-Length");
        const std::string chosen=sizeHeader.empty()?lengthHeader:sizeHeader;
        const double total=chosen.empty()?0.:std::stod(chosen);
        const auto body=response.get("body");
        const double loaded=double(body.isUndefined()?0:body.toString().size());
        auto event=JSValue::object();
        event.set("type","progress");
        event.set("lengthComputable",total!=0.);
        event.set("loaded",loaded);
        event.set("total",total);
        onProgress.call(self,{event});
    }
    const auto data=responseBody(response,responseType,mimeType);
    Cache.add(key,data);
    loading.set(urlText,JSValue{});
    const auto onLoad=arg(args,1);
    if(!onLoad.isUndefined())onLoad.call(self,{data});
    manager.itemEnd(url);
    return {};
}
JSValue method(const JSValue& receiverValue,const Args& args,const Args& captures){
    JSValue receiver=receiverValue;
    const auto name=captures[0].string();
    if(name=="load")return fileLoad(receiver,args);
    if(name=="abort"){
        auto controller=receiver.get("_abortController");
        if(!controller.isUndefined())controller.set("aborted",true);
        return receiver;
    }
    receiver.set(name=="setResponseType"?"responseType":"mimeType",arg(args,0));
    return receiver;
}
}
void FileLoader::setTransport(JSValue value){transportSlot()=std::move(value);}
JSValue FileLoader::transport(){return transportSlot();}
JSValue FileLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& selfValue,const Args& args,const Args&)->JSValue{
            JSValue self=selfValue;
            initialize(self,arg(args,0));
            self.set("mimeType","");
            self.set("responseType","");
            auto controller=JSValue::object();
            controller.set("aborted",false);
            self.set("_abortController",controller);
            return {};
        },{},"FileLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
                {"load",4},{"setResponseType",1},{"setMimeType",1},{"abort",0}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function(method,{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
FileLoader::FileLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);object_.set("mimeType","");object_.set("responseType","");auto controller=JSValue::object();controller.set("aborted",false);object_.set("_abortController",controller);}
FileLoader& FileLoader::setResponseType(JSValue value){object_.get("setResponseType").call(object_,{value});return *this;}
FileLoader& FileLoader::setMimeType(JSValue value){object_.get("setMimeType").call(object_,{value});return *this;}
FileLoader& FileLoader::abort(){object_.get("abort").call(object_);return *this;}
JSValue FileLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){return object_.get("load").call(object_,{url,onLoad,onProgress,onError});}
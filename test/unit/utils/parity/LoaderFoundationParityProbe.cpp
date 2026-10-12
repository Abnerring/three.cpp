#include "loaders/Cache.h"
#include "loaders/LoaderUtils.h"
#include "loaders/LoadingManager.h"
#include "core/AbortController.h"
#include "animation/AnimationUtils.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
JSValue call(JSValue self,const char* method,Args args={}){return self.get(method).call(self,args);}
void push(JSValue array,JSValue value){call(array,"push",{value});}
JSValue list(std::initializer_list<JSValue> values){auto array=JSValue::array();for(const auto& value:values)push(array,value);return array;}
JSValue encode(JSValue value) {
    if(value.isUndefined())return "#undefined";
    if(value.isCallable())return "#function";
    if(!value.isObject())return value;
    auto output=value.type()==JSValue::Type::Array?JSValue::array():JSValue::object();
    for(const auto& key:value.keys())output.set(key,encode(value.get(key)));return output;
}
JSValue callback(const JSValue& self,const Args& args,const Args& captures) {
    const auto scope=captures[0],trace=captures[1];auto state=captures[2];const auto name=captures[3];const auto mode=captures[4].toNumber();
    auto row=list({name,self.sameValue(scope)});for(const auto& value:args)push(row,value);push(trace,row);
    if(!state.get("used").toBoolean()) {
        if(mode==1&&name.strictEquals("start")){state.set("used",true);call(scope,"itemStart",{"inner"});}
        if(mode==2&&name.strictEquals("progress")){state.set("used",true);call(scope,"itemStart",{"inner"});}
        if(mode==3&&name.strictEquals("load")){state.set("used",true);call(scope,"itemStart",{"inner"});}
    }
    return {};
}
JSValue managerCase(int mode) {
    LoadingManager manager;const auto scope=manager.object();auto trace=JSValue::array(),state=JSValue::object();
    for(const auto& [property,name]:std::initializer_list<std::pair<const char*,const char*>>{{"onStart","start"},{"onLoad","load"},{"onProgress","progress"},{"onError","error"}})
        {auto mutableScope=scope;mutableScope.set(property,JSValue::function(callback,{scope,trace,state,name,mode}));}
    call(scope,"itemStart",{"a"});call(scope,"itemStart",{"b"});call(scope,"itemEnd",{"a"});call(scope,"itemError",{"b"});call(scope,"itemEnd",{"b"});
    if(mode>0)call(scope,"itemEnd",{"inner"});call(scope,"itemStart",{"next"});call(scope,"itemEnd",{"next"});
    auto foreign=JSValue::object();scope.get("itemStart").call(foreign,{"detached"});scope.get("itemEnd").call(foreign,{"detached"});return trace;
}
JSValue handlerCase() {
    LoadingManager manager;auto trace=JSValue::array(),first=JSValue::object(),second=JSValue::object();
    first.set("global",true);first.set("lastIndex",17);second.set("global",false);
    auto test=JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        push(captures[0],list({captures[1],self.get("lastIndex"),args[0]}));auto mutableSelf=self;mutableSelf.set("lastIndex",19);return args[0].strictEquals(captures[1]);
    },{trace,"first"});first.set("test",test);
    second.set("test",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {push(captures[0],list({"second",self.get("lastIndex"),args[0]}));return true;},{trace}));
    manager.addHandler(first,"one");manager.addHandler(second,"two");
    auto results=list({manager.getHandler("first"),manager.getHandler("first"),manager.getHandler("other")});manager.removeHandler(first);push(results,manager.getHandler("first"));manager.removeHandler(second);push(results,manager.getHandler("none"));
    return list({results,trace});
}
JSValue abortCase() {
    LoadingManager manager;auto first=manager.abortController.value(),signal=first.get("signal"),trace=JSValue::array();
    const auto append=JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue{push(captures[0],list({captures[1],self.sameValue(captures[2]),args[0].get("type"),self.get("aborted"),self.get("reason").get("name")}));return {};},{trace,"listener",signal});
    call(signal,"addEventListener",{"abort",append});call(signal,"addEventListener",{"abort",append});
    signal.set("onabort",JSValue::function([](const JSValue& self,const Args&,const Args& captures)->JSValue{push(captures[0],list({"onabort",self.sameValue(captures[1])}));return {};},{trace,signal}));
    auto same=manager.abortController.value().sameValue(first);manager.abort();auto cleared=manager._abortController.isNull();auto second=manager.abortController.value();call(first,"abort",{"ignored"});
    return list({same,cleared,!first.sameValue(second),second.get("signal").get("aborted"),trace,signal.get("reason").get("name")});
}
JSValue run(JSValue request) {
    const auto mode=request.get("mode").toString();
    if(mode=="nfc"){static thread_local LoadingManager manager;return manager.resolveURL(request.get("value"));}
    if(mode=="resolve")return LoaderUtils::resolveURL(request.get("url"),request.get("path"));
    if(mode=="base")return LoaderUtils::extractUrlBase(request.get("url"));
    if(mode=="cache") {
        Cache.clear();Cache.enabled=request.get("enabled");Cache.add(request.get("key"),request.get("value"));auto before=Cache.files.value();
        auto result=list({Cache.get(request.get("key")),JSValue::fromJSON(before.keys().empty()?JsonValue::Array{}:JsonValue::Array{before.keys()[0]})});
        Cache.remove(request.get("key"));push(result,Cache.get(request.get("key")));Cache.clear();push(result,!Cache.files.value().sameValue(before));return result;
    }
    if(mode=="manager")return managerCase(int(request.get("variant").toNumber()));
    if(mode=="handler")return handlerCase();
    if(mode=="abort")return abortCase();
    throw std::runtime_error("Unknown probe case");
}
}
int main() {
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto output=JSValue::array();
    for(std::uint32_t i=0;i<requests.length();++i)try{push(output,encode(run(requests.get(std::to_string(i)))));}catch(const JSTypeError&){push(output,"TypeError");}catch(const std::exception&){push(output,"Error");}
    std::cout<<output.toJSON()->dump();
}

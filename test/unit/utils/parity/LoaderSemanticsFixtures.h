#pragma once
#include "loaders/nodes/NodeLoader.h"
#include "loaders/nodes/NodeMaterialLoader.h"
#include "loaders/MaterialLoader.h"
#include "loaders/FileLoader.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "materials/nodes/MeshBasicNodeMaterial.h"
#include "nodes/core/ConstNode.h"
#include "utils.h"
#include <functional>

namespace LoaderSemanticsFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline JSValue list(const Args& values={}){auto a=JSValue::array();for(const auto& v:values)a.set(std::to_string(a.length()),v);return a;}
inline void push(JSValue a,const JSValue& v){a.set(std::to_string(a.length()),v);}
inline JSValue call(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
inline void getter(JSValue value,const JSValue& key,JSValue fn,bool enumerable=true){JSPropertyDescriptor d;d.getter=fn;d.enumerable=enumerable;d.configurable=true;value.defineProperty(key,d);}
inline JSValue outcome(const std::function<JSValue()>& fn,const JSValue& reason={}){
    try{return fn();}catch(const JSTypeError&){return "TypeError";}catch(...){const auto e=nativeExceptionValue(std::current_exception());return e.sameValue(reason)?JSValue("reason"):JSValue("other");}
}
struct Restore {
    JSValue object,key;std::optional<JSPropertyDescriptor> before;
    Restore(JSValue o,JSValue k):object(o),key(k),before(o.getOwnPropertyDescriptor(k)){}
    ~Restore(){if(before)object.defineProperty(key,*before);else object.erase(key);}
};
inline JSValue constructor(const std::string& module){return module=="NodeLoader"?nativeNodeLoaderConstructor():module=="NodeMaterialLoader"?nativeNodeMaterialLoaderConstructor():nativeMaterialLoaderConstructor();}
inline JSValue metadata(const std::string& module){
    const auto ctor=constructor(module),p=ctor.get("prototype");auto out=object(),methods=object();
    out.set("name",ctor.get("name"));out.set("length",ctor.get("length"));out.set("source",call(ctor,"toString"));
    const Args names=module=="NodeLoader"?Args{"load","parseNodes","parse","setTextures","setNodes","createNodeFromType"}:module=="NodeMaterialLoader"?Args{"parse","setNodes","setNodeMaterials","createMaterialFromType"}:Args{"load","parse","setTextures","createMaterialFromType"};
    for(const auto& name:names){const auto d=p.getOwnPropertyDescriptor(name);auto m=object();m.set("name",d->value->get("name"));m.set("length",d->value->get("length"));m.set("source",call(*d->value,"toString"));m.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));methods.set(name,m);}
    if(module=="MaterialLoader")for(const auto* name:{"createMaterialFromType","registerMaterial"}){auto m=object();const auto f=ctor.get(name);m.set("name",f.get("name"));m.set("length",f.get("length"));m.set("source",call(f,"toString"));methods.set(std::string("static:")+name,m);}
    out.set("methods",methods);const auto loader=ctor.construct();out.set("texturesOrdinary",loader.get("textures").getPrototype().sameValue(JSValue::object().getPrototype()));
    if(module!="MaterialLoader")out.set("nodesOrdinary",loader.get("nodes").getPrototype().sameValue(JSValue::object().getPrototype()));
    if(module=="NodeMaterialLoader")out.set("materialsOrdinary",loader.get("nodeMaterials").getPrototype().sameValue(JSValue::object().getPrototype()));
    return out;
}
inline JSValue iteratorScenario(const std::string& variant){
    auto loader=nativeNodeLoaderConstructor().construct(),trace=list(),reason=object(),state=object(),data=object();state.set("pass",0);data.set("uuid","u");data.set("type","T");
    auto deserialize=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        push(c[0],list({"deserialize",self.get("uuid"),args[0].get("meta").get("nodes").getPrototype().sameValue(JSValue::object().getPrototype()),args[0].get("meta").getPrototype().sameValue(JSValue::object().getPrototype())}));
        if(c[1].strictEquals("deserializeThrow"))throw JSThrownValue(c[2]);return {};
    },{trace,variant,reason});
    loader.set("createNodeFromType",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        push(c[0],list({"create",args[0]}));if(c[1].strictEquals("createThrow")||c[1].strictEquals("returnThrow"))throw JSThrownValue(c[2]);auto n=object();n.set("deserialize",c[3]);return n;
    },{trace,variant,reason,deserialize}));
    for(const auto* name:{"uuid","type"}){
        getter(data,name,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],std::string("get:")+c[1].toString());if(c[2].strictEquals(std::string(c[1].toString())+"Throw"))throw JSThrownValue(c[3]);return c[1].strictEquals("uuid")?JSValue("u"):JSValue("T");},{trace,name,variant,reason}));
    }
    auto iterable=object();iterable.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        auto state=c[0];const auto pass=state.get("pass").toNumber()+1;state.set("pass",pass);push(c[1],list({"iterator",pass,self.sameValue(c[2])}));auto it=object(),cursor=object();cursor.set("i",0);
        it.set("next",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
            const auto i=c[0].get("i").toNumber();auto cursor=c[0];cursor.set("i",i+1);push(c[1],list({"next",c[2],i,self.sameValue(c[6])}));
            if(c[3].strictEquals("nextThrow")||(c[3].strictEquals("secondNextThrow")&&c[2].strictEquals(2)))throw JSThrownValue(c[4]);
            auto result=object();result.set("done",i>0);result.set("value",c[5]);return result;
        },{cursor,c[1],pass,c[3],c[4],c[5],it}));
        getter(it,"return",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:return",c[1],self.sameValue(c[4])}));if(c[2].strictEquals("returnThrow"))throw JSThrownValue("close failed");return JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"return",c[1],self.sameValue(c[2])}));return object();},{c[0],c[1],c[4]});},{c[1],pass,c[3],c[4],it}));
        return it;
    },{state,trace,iterable,variant,reason,data}));
    auto out=object();out.set("result",outcome([&](){const auto nodes=call(loader,"parseNodes",{iterable});return nodes.get("u").get("uuid");},reason));out.set("trace",trace);out.set("metaRetained",data.hasOwn("meta"));return out;
}
inline JSValue registryScenario(){
    auto loader=nativeNodeLoaderConstructor().construct(),trace=list(),state=object(),first=object(),second=object();state.set("reads",0);
    const auto key=JSValue::symbol("type");second.set(key,JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"construct");auto out=object();out.set("tag","second");return out;},{trace}));
    getter(first,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"first:type");return c[1];},{trace,JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return object();})}));
    getter(loader,"nodes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto s=c[0];const auto n=s.get("reads").toNumber();s.set("reads",n+1);push(c[1],"get:nodes");return c[n==0?2:3];},{state,trace,first,second}));
    auto out=object();out.set("tag",call(loader,"createNodeFromType",{key}).get("tag"));out.set("trace",trace);return out;
}
inline JSValue keyScenario(){
    auto loader=nativeNodeLoaderConstructor().construct(),trace=list(),key=object(),data=object(),reason=object();
    key.set("toString",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"key");return "u";},{trace}));data.set("uuid",key);data.set("type","T");
    getter(loader,"createNodeFromType",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:create");return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"create");auto n=object();getter(n,"uuid",JSValue{},true);JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"set:uuid");return {};},{c[0]});d.configurable=true;n.defineProperty("uuid",d);n.set("deserialize",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"deserialize");return {};},{c[0]}));return n;},{c[0]});},{trace}));
    call(loader,"parseNodes",{list({data})});return trace;
}
inline JSValue arrayScenario(const std::string& variant){
    auto loader=nativeNodeLoaderConstructor().construct(),trace=list(),state=object(),first=object(),second=object();
    first.set("uuid",variant=="duplicate"?"same":"a");first.set("type","T");first.set("tag",1);second.set("uuid",variant=="duplicate"?"same":"b");second.set("type","T");second.set("tag",2);
    auto values=variant=="grow"?list({first}):list({first,second});state.set("count",0);
    loader.set("createNodeFromType",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto state=c[0];const auto id=state.get("count").toNumber()+1;state.set("count",id);push(c[1],list({"create",id}));
        if(c[2].strictEquals("grow")&&id==1)push(c[3],c[4]);
        auto node=object();node.set("deserialize",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"deserialize",c[1],a[0].get("tag")}));return {};},{c[1],id}));return node;
    },{state,trace,variant,values,second}));
    const auto nodes=call(loader,"parseNodes",{values});auto out=object();out.set("count",state.get("count"));out.set("keys",list([&](){Args keys;for(const auto& k:nodes.keys())keys.push_back(k);return keys;}()));out.set("trace",trace);return out;
}
inline JSValue factoryScenario(const std::string& variant){
    auto ctor=nativeMaterialLoaderConstructor(),loader=ctor.construct(),trace=list(),material=object();
    if(variant=="static"){
        Restore restore(ctor,"createMaterialFromType");
        getter(ctor,"createMaterialFromType",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:static",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:static",self.sameValue(c[1]),a[0]}));return c[2];},{c[0],c[1],c[2]});},{trace,ctor,material}));
        auto out=object();out.set("same",call(loader,"createMaterialFromType",{"custom"}).sameValue(material));out.set("trace",trace);return out;
    }
    const auto key=JSValue::symbol("registered"),klass=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"construct");return c[1];},{trace,material});
    auto out=object();out.set("registerUndefined",call(ctor,"registerMaterial",{key,klass}).isUndefined());out.set("same",call(loader,"createMaterialFromType",{key}).sameValue(material));out.set("trace",trace);return out;
}
inline JSValue forInScenario(const std::string& variant){
    auto loader=nativeNodeMaterialLoaderConstructor().construct(),trace=list(),material=object(),nodes=object(),proto=object(),input=JSValue::object(proto),reason=object();
    for(const auto* key:{"a","b","c","x","y"})nodes.set(key,std::string("node:")+key);
    material.set("fromJSON",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{push(c[0],"fromJSON");return self;},{trace}));
    loader.set("createMaterialFromType",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{material}));loader.set("nodes",nodes);
    proto.set("inherited","b");proto.set("hidden","c");input.set("own","a");JSPropertyDescriptor hidden;hidden.value="x";hidden.enumerable=false;input.defineProperty("hidden",hidden);
    input.set(JSValue::symbol("ignored"),"y");
    if(variant=="mutate"){
        proto.set("deleted","c");input.set("deleted","x");
        getter(input,"own",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:own");auto input=c[1];input.erase("deleted");input.set("added","y");return "a";},{trace,input}));
    }
    if(variant=="throw")getter(input,"own",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{throw JSThrownValue(c[0]);},{reason}));
    auto json=object();json.set("type","T");json.set("inputNodes",variant=="string"?JSValue("ab"):variant=="null"?JSValue(nullptr):variant=="undefined"?JSValue{}:input);
    auto out=object();out.set("result",outcome([&](){return call(loader,"parse",{json}).sameValue(material);},reason));
    auto assigned=object();for(const auto& key:material.keys())if(key!="fromJSON")assigned.set(key,material.get(key));out.set("assigned",assigned);out.set("trace",trace);return out;
}
inline JSValue dynamicSuperScenario(){
    auto ctor=nativeNodeMaterialLoaderConstructor(),home=ctor.get("prototype"),base=home.getPrototype(),replacement=object(),loader=ctor.construct(),trace=list(),material=object();
    struct SuperRestore{JSValue home,old;~SuperRestore(){home.setPrototype(old);}}restore{home,base};
    getter(replacement,"parse",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:super",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:super",self.sameValue(c[1]),a[0].get("tag")}));return c[2];},{c[0],c[1],c[2]});},{trace,loader,material}));
    getter(replacement,"createMaterialFromType",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:create",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:create",self.sameValue(c[1]),a[0]}));return c[2];},{c[0],c[1],c[2]});},{trace,loader,material}));
    home.setPrototype(replacement);auto json=object();json.set("tag","json");
    auto out=object();out.set("parseSame",call(loader,"parse",{json}).sameValue(material));out.set("createSame",call(loader,"createMaterialFromType",{"missing"}).sameValue(material));out.set("trace",trace);return out;
}
inline JSValue materialParseScenario(const std::string& variant){
    auto loader=nativeMaterialLoaderConstructor().construct(),json=object(),trace=list(),material=object(),reason=object(),textures=object();
    getter(loader,"createMaterialFromType",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],"get:create");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"create",self.sameValue(c[1]),a[0]}));return c[2];},{c[0],c[1],c[2]});},{trace,loader,material}));
    getter(json,"type",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:type");return "T";},{trace}));
    getter(loader,"textures",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:textures");return c[1];},{trace,textures}));
    if(variant!="missing")getter(material,"fromJSON",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:fromJSON");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"fromJSON",self.sameValue(c[1]),a[0].sameValue(c[2]),a[1].sameValue(c[3])}));if(c[4].strictEquals("throw"))throw JSThrownValue(c[5]);return "ignored";},{c[0],c[1],c[2],c[3],c[4],c[5]});},{trace,material,json,textures,variant,reason}));
    auto out=object();out.set("result",outcome([&](){return call(loader,"parse",{json}).sameValue(material);},reason));out.set("trace",trace);return out;
}
inline JSValue loadScenario(const std::string& module,const std::string& variant){
    auto file=FileLoader::prototype();Restore restore(file,"load");auto trace=list(),reason=object(),manager=object(),loader=constructor(module).construct({manager}),parsed=object(),state=object();
    manager.set("itemError",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"itemError",self.sameValue(c[1]),a[0]}));return {};},{trace,manager}));
    loader.set("path","base/");auto headers=object();headers.set("custom","yes");loader.set("requestHeader",headers);loader.set("withCredentials",true);
    getter(loader,"parse",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],"get:parse");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"parse",self.sameValue(c[1]),a[0].get("value")}));if(c[2].strictEquals("parseThrow")||c[2].strictEquals("onErrorThrow"))throw JSThrownValue(c[3]);return c[4];},{c[0],c[1],c[2],c[3],c[4]});},{trace,loader,variant,reason,parsed}));
    auto context=object();context.set("loader",loader);context.set("entered",false);
    auto onLoad=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"onLoad",self.isUndefined(),a[0].sameValue(c[1])}));if(c[2].strictEquals("onLoadThrow"))throw c[3];if(c[2].strictEquals("reenter")&&!c[4].get("entered").toBoolean()){auto context=c[4];context.set("entered",true);call(context.get("loader"),"load",{"nested.json",context.get("onLoad"),context.get("progress"),context.get("onError")});}return {};},{trace,parsed,variant,reason,context});
    auto onError=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"onError",self.isUndefined(),a[0].sameValue(c[1])}));if(c[2].strictEquals("onErrorThrow"))throw JSThrownValue(c[1]);return {};},{trace,reason,variant});
    auto progress=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};});
    context.set("onLoad",onLoad);context.set("onError",onError);context.set("progress",progress);
    auto jsonObject=nativeGlobalBinding("JSON");Restore restoreJSON(jsonObject,"parse");const auto originalJSON=jsonObject.get("parse");
    jsonObject.set("parse",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"JSON.parse",self.sameValue(c[1]),a[0]}));return c[2].call(self,a);},{trace,jsonObject,originalJSON}));
    file.set("load",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
        push(c[0],list({"file.load",a[0],self.get("path"),self.get("requestHeader").sameValue(c[1]),self.get("withCredentials"),a[2].sameValue(c[2]),a[3].sameValue(c[3])}));
        auto state=c[4];state.set("callbackName",a[1].get("name"));state.set("callbackLength",a[1].get("length"));state.set("callbackSource",call(a[1],"toString"));
        a[1].call(self,{"{\"value\":7}"});return "ignored file return";
    },{trace,headers,progress,onError,state}));
    auto out=object();out.set("result",outcome([&](){return call(loader,"load",{"asset.json",onLoad,progress,onError}).isUndefined();},reason));out.set("trace",trace);out.set("callback",state);return out;
}
inline JSValue realRoundtrip(){
    const auto loader=nativeNodeMaterialLoaderConstructor().construct();auto nodes=object(),registry=object(),json=object();
    auto value=nativeConstNodeConstructor().construct({.375,"float"});nodes.set("node-1",value);registry.set("MeshBasicNodeMaterial",nativeMeshBasicNodeMaterialConstructor());call(loader,"setNodes",{nodes});call(loader,"setNodeMaterials",{registry});
    auto input=object();input.set("opacityNode","node-1");json.set("type","MeshBasicNodeMaterial");json.set("uuid","material-1");json.set("opacity",.75);json.set("inputNodes",input);
    const auto material=call(loader,"parse",{json});auto out=object();out.set("isNodeMaterial",material.get("isNodeMaterial"));out.set("uuid",material.get("uuid"));out.set("opacity",material.get("opacity"));out.set("sameNode",material.get("opacityNode").sameValue(value));out.set("value",material.get("opacityNode").get("value"));return out;
}
inline JSValue run(const JSValue& request){
    const auto mode=request.get("mode").toString(),variant=request.get("variant").isUndefined()?std::string{}:request.get("variant").toString();
    if(mode=="metadata")return metadata(request.get("module").toString());
    if(mode=="iterator")return iteratorScenario(variant);
    if(mode=="registry")return registryScenario();
    if(mode=="array")return arrayScenario(variant);
    if(mode=="factory")return factoryScenario(variant);
    if(mode=="keys")return keyScenario();
    if(mode=="forIn")return forInScenario(variant);
    if(mode=="super")return dynamicSuperScenario();
    if(mode=="parse")return materialParseScenario(variant);
    if(mode=="load")return loadScenario(request.get("module").toString(),variant);
    if(mode=="real")return realRoundtrip();
    if(mode=="realNode"){
        auto loader=nativeNodeLoaderConstructor().construct(),registry=object(),data=object();registry.set("ConstNode",nativeConstNodeConstructor());call(loader,"setNodes",{registry});data.set("uuid","node-1");data.set("type","ConstNode");return outcome([&](){return call(loader,"parseNodes",{list({data})});});
    }
    if(mode=="nullish"){
        const auto loader=nativeNodeLoaderConstructor().construct();auto out=object();
        out.set("undefined",call(loader,"parseNodes").getPrototype().sameValue(JSValue::object().getPrototype()));out.set("null",outcome([&](){return call(loader,"parseNodes",{nullptr});}));out.set("number",outcome([&](){return call(loader,"parseNodes",{3});}));
        call(loader,"setTextures");call(loader,"setNodes");out.set("texturesUndefined",loader.get("textures").isUndefined());out.set("nodesUndefined",loader.get("nodes").isUndefined());return out;
    }
    throw JSTypeError("Unknown loader semantics scenario");
}
inline JSValue request(const char* mode,const char* variant="",const char* module="NodeLoader"){auto r=object();r.set("mode",mode);r.set("variant",variant);r.set("module",module);return r;}
}

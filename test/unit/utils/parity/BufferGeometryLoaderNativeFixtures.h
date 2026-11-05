#pragma once
#include "ObjectLoaderNativeFixtures.h"
#include "loaders/BufferGeometryLoader.h"
#include "core/BufferGeometry.h"
namespace BufferGeometryLoaderNativeFixtures {
using namespace ObjectLoaderNativeFixtures;
inline JSValue metadata(){
    const auto ctor=nativeBufferGeometryLoaderConstructor(),prototype=ctor.get("prototype");auto out=object(),methods=object();
    out.set("name",ctor.get("name"));out.set("length",ctor.get("length"));out.set("source",call(ctor,"toString"));out.set("super",ctor.getPrototype().sameValue(nativeLoaderConstructor()));
    auto names=list();for(const auto& name:prototype.ownPropertyNames())push(names,name);out.set("prototypeNames",names);
    for(const auto& name:prototype.ownPropertyNames())if(name!="constructor"){
        const auto d=prototype.getOwnPropertyDescriptor(name);const auto fn=d->value.value();auto row=object();
        row.set("name",fn.get("name"));row.set("length",fn.get("length"));row.set("source",call(fn,"toString"));row.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));row.set("constructible",fn.isConstructible());methods.set(name,row);
    }out.set("methods",methods);return out;
}
inline JSValue run(const JSValue& request){
    if(request.get("op").strictEquals("metadata"))return metadata();
    Snapshot snapshot(request);auto events=list(),result=object();const auto loader=nativeBufferGeometryLoaderConstructor().construct();auto json=request.get("json");
    if(request.get("variant").strictEquals("dataGetter")){
        const auto data=json.get("data");getter(json,"data",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"data");return c[1];},{events,data}));
    }
    if(request.get("variant").strictEquals("inherited"))json.get("data").get("attributes").setPrototype(request.get("inherited"));
    if(request.get("variant").strictEquals("reenter")){
        auto normal=json.get("data").get("attributes").get("normal");
        getter(normal,"data",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            const auto geometry=call(c[0],"parse",{c[1]});push(c[2],list({"reenter",geometry.get("attributes").get("position").get("data").get("array").get("0")}));return "ib";
        },{loader,request.get("nested"),events}));
    }
    try{result.set("value",snapshot.value(call(loader,"parse",{json})));}
    catch(...){const auto error=nativeExceptionValue(std::current_exception());auto row=object();if(error.isObject())row.set("name",error.get("name"));else{row.set("name","ThrownValue");row.set("raw",error);}result.set("error",row);}
    result.set("events",events);return result;
}
}

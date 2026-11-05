#pragma once
#include "loaders/ObjectLoader.h"
#include "loaders/ObjectLoaderShapes.h"
#include "loaders/FileLoader.h"
#include "loaders/ImageLoader.h"
#include "loaders/MaterialLoader.h"
#include "loaders/Cache.h"
#include "core/JSGlobal.h"
#include "core/JSAsyncFunction.h"
#include "utils.h"
#include <set>
#include <algorithm>
#include <functional>
#include <cmath>

namespace ObjectLoaderNativeFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline void push(JSValue array,const JSValue& value){array.set(JSValue(array.length()),value);}
inline JSValue list(const Args& values={}){auto result=JSValue::array();for(const auto& v:values)push(result,v);return result;}
inline JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
inline JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
struct Restore {
    JSValue object,key;std::optional<JSPropertyDescriptor> previous;
    Restore(JSValue o,JSValue k):object(o),key(k),previous(o.getOwnPropertyDescriptor(k)){}
    ~Restore(){if(previous)object.defineProperty(key,*previous);else object.erase(key);}
};
inline void getter(JSValue target,const char* key,JSValue body){JSPropertyDescriptor d;d.getter=body;d.configurable=d.enumerable=true;target.defineProperty(key,d);}
inline JSValue await(const JSValue& promise) {
    auto state=object();state.set("settled",false);state.set("rejected",false);
    const auto reaction=[](bool rejected,const JSValue& state) {
        return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto state=c[0];state.set("settled",true);state.set("rejected",c[1]);state.set("value",arg(a,0));return {};},{state,rejected});
    };
    awaitNativePromise(promise,reaction(false,state),reaction(true,state));
    for(unsigned i=0;!state.get("settled").toBoolean()&&i<100;++i) {
        if(hasPendingMicrotasks())drainMicrotasks();else if(hasPendingHostTasks())drainHostTasks();else break;
    }
    if(!state.get("settled").toBoolean())throw std::runtime_error("ObjectLoader fixture promise did not settle");
    if(state.get("rejected").toBoolean())throw JSThrownValue(state.get("value"));return state.get("value");
}
inline JSValue metadata() {
    const auto ctor=nativeObjectLoaderConstructor(),p=ctor.get("prototype");auto result=object(),methods=object();
    result.set("name",ctor.get("name"));result.set("length",ctor.get("length"));result.set("source",call(ctor,"toString"));
    result.set("prototypeNames",list([&]{Args names;for(const auto& name:p.ownPropertyNames())names.emplace_back(name);return names;}()));
    for(const auto& name:p.ownPropertyNames()) {
        if(name=="constructor")continue;const auto d=p.getOwnPropertyDescriptor(name);const auto f=d->value.value();auto entry=object();
        entry.set("name",f.get("name"));entry.set("length",f.get("length"));entry.set("source",call(f,"toString"));
        entry.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));
        entry.set("async",f.getPrototype().sameValue(nativeAsyncFunctionPrototype()));entry.set("constructible",f.isConstructible());methods.set(name,entry);
    }
    const auto f=ctor.get("registerGeometry");auto entry=object();entry.set("name",f.get("name"));entry.set("length",f.get("length"));entry.set("source",call(f,"toString"));methods.set("static:registerGeometry",entry);
    result.set("methods",methods);const auto loader=ctor.construct();auto defaults=object();
    for(const auto* name:{"crossOrigin","withCredentials","path","resourcePath","requestHeader"})defaults.set(name,loader.get(name));result.set("defaults",defaults);return result;
}
struct Snapshot {
    JSValue schema,brands,structuralBrands;std::set<std::string> uuids;Args seen;
    explicit Snapshot(const JSValue& request):schema(request.get("_schema")),brands(request.get("_brands")),structuralBrands(request.get("_structuralBrands")){collect(request);}
    void collect(const JSValue& value) {
        if(!value.isObject())return;
        for(const auto& key:value.keys()) {
            if(key=="_schema"||key=="_brands"||key=="_structuralBrands")continue;
            const auto child=value.get(key);
            if(key=="uuid"&&child.type()==JSValue::Type::String)uuids.insert(child.string());
            collect(child);
        }
    }
    JSValue value(const JSValue& source) {
        if(source.isUndefined()){auto o=object();o.set("$undefined",true);return o;}
        if(source.type()==JSValue::Type::Number && !std::isfinite(source.number())){auto o=object();o.set("$number",std::isnan(source.number())?"NaN":source.number()<0?"-Infinity":"Infinity");return o;}
        if(!source.isObject())return source;
        for(std::size_t i=0;i<seen.size();++i)if(seen[i].sameValue(source)){auto o=object();o.set("$ref",i+1);return o;}
        seen.push_back(source);auto result=object();result.set("$id",seen.size());
        if(source.type()==JSValue::Type::Array) {
            result.set("$type","Array");auto items=list();for(double i=0;i<source.get("length").toNumber();++i)push(items,value(source.get(JSValue(i))));result.set("items",items);return result;
        }
        for(const auto* name:{"Int8Array","Uint8Array","Uint8ClampedArray","Int16Array","Uint16Array","Int32Array","Uint32Array","Float32Array","Float64Array"}) {
            if(source.instanceOf(nativeGlobalBinding(name))) {
                result.set("$type",name);auto items=list();for(double i=0;i<source.get("length").toNumber();++i)push(items,value(source.get(JSValue(i))));result.set("items",items);return result;
            }
        }
        std::string brand="Object";
        if(!brands.isUndefined())for(double i=0;i<brands.length();++i) {
            const auto candidate=brands.get(JSValue(i));
            if(source.get("is"+candidate.toString()).toBoolean()){brand=candidate.toString();break;}
        }
        if(brand=="Object" && !structuralBrands.isUndefined())for(double i=0;i<structuralBrands.length();++i) {
            const auto rule=structuralBrands.get(JSValue(i));
            if(source.has(rule.get(JSValue(0))) && source.has(rule.get(JSValue(1)))) {brand=rule.get(JSValue(2)).toString();break;}
        }
        result.set("$type",brand);
        if(brand=="Object") {
            auto keys=source.keys();std::sort(keys.begin(),keys.end());
            for(const auto& key:keys) {
                if(key=="id")continue;const auto child=source.get(key);
                if(key=="uuid" && (child.type()!=JSValue::Type::String||!uuids.contains(child.string())))continue;
                if(child.isCallable())continue;result.set(key,value(child));
            }
        } else {
            const auto fields=schema.get(brand);
            for(double i=0;i<fields.length();++i) {
                const auto key=fields.get(JSValue(i));if(!source.has(key))continue;const auto child=source.get(key);
                if(key.strictEquals("uuid") && (child.type()!=JSValue::Type::String||!uuids.contains(child.string())))continue;
                result.set(key,value(child));
            }
        }
        return result;
    }
};
inline JSValue dispatchJSON(const JSValue& events,const char* tag,const JSValue& variant) {
    auto json=object();
    for(const auto* key:{"animations","shapes","geometries","images","textures","materials","object","skeletons"})
        getter(json,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],list({"get",c[1],c[2]}));
            if(c[1].strictEquals("outer")&&c[2].strictEquals("geometries")&&c[3].strictEquals("getterThrow"))throw JSThrownValue("geometries");
            auto value=object();value.set("tag",c[1].toString()+":"+c[2].toString());return value;
        },{events,tag,key,variant}));
    return json;
}
inline JSValue dispatch(const JSValue& loader,const JSValue& request,const JSValue& events,const JSValue& onLoad) {
    const auto variant=request.get("variant");auto state=object();state.set("inner",dispatchJSON(events,"inner",variant));
    for(const auto* name:{"parseAnimations","parseShapes","parseGeometries","parseImages","parseTextures","parseMaterials","parseObject","parseSkeletons","bindSkeletons","bindLightTargets"}) {
        getter(loader,name,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],list({"method",c[1]}));
            return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
                auto labels=list();for(const auto& v:a)push(labels,v.isUndefined()?JSValue("undefined"):v.isCallable()?JSValue("callback"):v.get("tag").isUndefined()?JSValue("undefined"):v.get("tag"));
                push(c[0],list({"call",c[1],labels}));
                if(c[1].strictEquals("parseImages")&&c[2].strictEquals("syncImages"))arg(a,1).call();
                if(c[1].strictEquals("bindSkeletons")||c[1].strictEquals("bindLightTargets"))return {};
                auto result=object();
                if(c[1].strictEquals("parseObject")) {
                    const auto input=arg(a,0).get("tag");const bool outer=input.strictEquals("outer:object");
                    if(outer&&c[2].strictEquals("reenter")) {
                        const auto inner=call(self,"parse",{c[3].get("inner")});push(c[0],list({"reenter",inner.get("tag")}));
                    }
                    result.set("tag",outer?JSValue("outer"):JSValue("inner"));
                } else result.set("tag",c[1]);return result;
            },c);
        },{events,name,variant,state}));
    }
    return call(loader,"parse",{dispatchJSON(events,"outer",variant),request.get("onLoad").toBoolean()?onLoad:JSValue{}});
}
inline JSValue geometryDispatch(const JSValue& loader,const JSValue& request,const JSValue& events) {
    const auto variant=request.get("variant");const JSValue type=variant.strictEquals("unknown")?"ObjectLoaderParityUnknown":"ObjectLoaderParityCustom";
    auto shared=object(),shapes=object();shared.set("payload",17);shapes.set("tag","shapes");
    const auto makeData=[&](const char* owner) {
        auto data=object();
        for(const auto* key:{"type","uuid","name","userData"})getter(data,key,JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {
            push(c[0],list({"get",c[1],c[2],self.sameValue(c[5])}));
            if(c[2].strictEquals("type"))return c[3];if(c[2].strictEquals("uuid"))return c[1];
            if(c[2].strictEquals("name"))return c[1].toString()+"-geometry";return c[4];
        },{events,owner,key,type,shared,data}));return data;
    };
    const auto outer=makeData("outer"),inner=makeData("inner");auto geometryClass=object();
    getter(geometryClass,"fromJSON",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {
        push(c[0],list({"get:fromJSON",self.sameValue(c[1])}));
        return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
            const bool outer=arg(a,0).sameValue(c[3]);const JSValue owner=outer?"outer":"inner";
            push(c[0],list({"fromJSON",owner,self.sameValue(c[1]),arg(a,1).sameValue(c[5])}));
            if(c[2].strictEquals("throw"))throw JSThrownValue("custom geometry");
            if(outer&&c[2].strictEquals("reenter")) {
                const auto nested=call(c[6],"parseGeometries",{list({c[4]}),c[5]});push(c[0],list({"reenter",nested.get("inner").get("marker")}));
            }
            auto result=object();result.set("marker",owner);return result;
        },c,"fromJSON",2);
    },{events,geometryClass,variant,outer,inner,shapes,loader}));
    if(!variant.strictEquals("unknown"))call(nativeObjectLoaderConstructor(),"registerGeometry",{type,geometryClass});
    return call(loader,"parseGeometries",{list({outer}),shapes});
}
inline JSValue materialDispatch(const JSValue& loader,const JSValue& request,const JSValue& events) {
    auto prototype=nativeMaterialLoaderConstructor().get("prototype");Restore restore(prototype,"parse");
    auto data=object(),key=object(),state=object();state.set("coercions",0);
    key.set("toString",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[0];const auto count=state.get("coercions").toNumber()+1;state.set("coercions",count);push(c[1],list({"key",count}));
        if(c[2].strictEquals("keyThrow")&&count==2)throw JSThrownValue("material key");return "material";
    },{state,events,request.get("variant")}));
    getter(data,"uuid",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"uuid");return c[1];},{events,key}));
    getter(prototype,"parse",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        push(c[0],"get:parse");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
            push(c[0],list({"parse",arg(a,0).sameValue(c[1])}));if(c[2].strictEquals("parseThrow"))throw JSThrownValue("material parse");
            auto result=object();result.set("marker",17);return result;
        },c,"parse",1);
    },{events,data,request.get("variant")}));
    return call(loader,"parseMaterials",{list({data}),object()});
}
inline JSValue loadScenario(const JSValue& loader,const JSValue& request,const JSValue& events,
                            const JSValue& onLoad,const JSValue& onError) {
    auto prototype=nativeFileLoaderConstructor().get("prototype");Restore restore(prototype,"load");
    auto header=object();header.set("X","fixture");auto self=loader;self.set("path","base/");self.set("resourcePath","");self.set("requestHeader",header);self.set("withCredentials",true);
    auto descriptor=*prototype.getOwnPropertyDescriptor("load");
    descriptor.value=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        push(c[0],list({"file.load",arg(a,0),self.get("path"),self.get("withCredentials"),self.get("requestHeader").sameValue(c[1])}));
        const auto body=c[2].strictEquals("badJSON")?JSValue("{"):call(nativeGlobalBinding("JSON"),"stringify",{c[3]});
        arg(a,1).call({}, {body});return {};
    },{events,header,request.get("variant"),request.get("json")},"load",4);prototype.defineProperty("load",descriptor);
    if(request.get("op").strictEquals("loadAsync"))return await(call(loader,"loadAsync",{"asset.json"}));
    call(loader,"load",{"asset.json",onLoad,{},onError});return {};
}
inline JSValue run(const JSValue& request) {
    const auto ctor=nativeObjectLoaderConstructor();
    if(request.get("op").strictEquals("metadata"))return metadata();
    auto globals=nativeGlobalObject();Restore imageGlobal(globals,"HTMLImageElement");
    if(!globals.has("HTMLImageElement"))globals.set("HTMLImageElement",JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"HTMLImageElement",0));
    Snapshot snapshot(request);auto events=list(),result=object(),callbackState=object();
    callbackState.set("count",0);callbackState.set("receiverUndefined",true);
    const auto console=ThreeUtils::getConsoleFunction();
    struct RestoreConsole {JSValue before;~RestoreConsole(){ThreeUtils::setConsoleFunction(before);}}restoreConsole{console};
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto entry=list({"console",arg(a,0),arg(a,1)});for(std::size_t i=2;i<a.size();++i)push(entry,c[1].toString().starts_with("load")?JSValue("argument"):arg(a,i));push(c[0],entry);return {};},{events,request.get("op")}));
    auto loader=ctor.construct();
    const auto options=request.get("options");
    if(!options.isUndefined())for(const auto& name:options.keys())loader.set(name,options.get(name));
    auto onLoad=JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
        auto state=c[0];state.set("count",state.get("count").toNumber()+1);state.set("value",arg(a,0));state.set("receiverUndefined",state.get("receiverUndefined").toBoolean()&&receiver.isUndefined());push(c[1],"onLoad");return {};
    },{callbackState,events});
    auto onError=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto state=c[0];state.set("error",arg(a,0));push(c[1],list({"onError",arg(a,0).isObject()?arg(a,0).get("name"):JSValue("ThrownValue")}));return {};},{callbackState,events});
    const auto op=request.get("op").toString();
    try {
        JSValue value;
        if(op=="dispatch")value=dispatch(loader,request,events,onLoad);
        else if(op=="geometryDispatch")value=geometryDispatch(loader,request,events);
        else if(op=="materialDispatch")value=materialDispatch(loader,request,events);
        else if(op=="load" || op=="loadAsync")value=loadScenario(loader,request,events,onLoad,onError);
        else if(op=="parse" || op=="parseAsync") {
            value=call(loader,op.c_str(),op=="parse"?Args{request.get("json"),request.get("onLoad").toBoolean()?onLoad:JSValue{}}:Args{request.get("json")});
            if(op=="parseAsync")value=await(value);
        } else if(op=="parseObject")value=call(loader,"parseObject",{request.get("json"),request.get("geometries"),request.get("materials"),request.get("textures"),request.get("animations")});
        else if(op=="parseShapes" || op=="parseAnimations")value=call(loader,op.c_str(),{request.get("json")});
        else if(op=="parseImages" || op=="parseImagesAsync") {
            value=call(loader,op.c_str(),op=="parseImages"?Args{request.get("json"),request.get("onLoad").toBoolean()?onLoad:JSValue{}}:Args{request.get("json")});
            if(op=="parseImagesAsync")value=await(value);
        } else if(op=="parseGeometries")value=call(loader,op.c_str(),{request.get("json"),request.get("shapes")});
        else if(op=="parseMaterials")value=call(loader,op.c_str(),{request.get("json"),request.get("textures")});
        else if(op=="parseTextures") {
            const auto images=call(loader,"parseImages",{request.get("images")});value=call(loader,op.c_str(),{request.get("json"),images});
        } else if(op=="parseSkeletons") {
            const auto root=call(loader,"parseObject",{request.get("object"),object(),object(),object(),object()});
            const auto skeletons=call(loader,"parseSkeletons",{request.get("json"),root});value=object();value.set("root",root);value.set("skeletons",skeletons);
        } else throw std::logic_error("Unknown ObjectLoader native fixture operation: "+op);
        if(op=="load")value=callbackState.get("value");
        result.set("value",snapshot.value(value));
        if(request.get("onLoad").toBoolean()) {
            result.set("onLoadCount",callbackState.get("count"));result.set("onLoadSame",callbackState.get("value").sameValue(value));result.set("receiverUndefined",callbackState.get("receiverUndefined"));
        }
    } catch(...) {
        const auto error=nativeExceptionValue(std::current_exception());auto normalized=object();
        if(error.isObject())normalized.set("name",error.get("name"));else {normalized.set("name","ThrownValue");normalized.set("raw",error);}
        result.set("error",normalized);
    }
    result.set("events",events);return result;
}
} // namespace

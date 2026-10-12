#include "core/EventDispatcher.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <cmath>

namespace {
using Args=std::vector<JSValue>;
void push(JSValue a,JSValue v){a.set(std::to_string(a.length()),v);}
JSValue list(std::initializer_list<JSValue> values){auto a=JSValue::array();for(const auto&v:values)push(a,v);return a;}
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> entries){auto o=JSValue::object();for(const auto&[k,v]:entries)o.set(k,v);return o;}
JSValue call(JSValue self,const std::string& name,const Args& a={}){return self.get(name).call(self,a);}
std::string error(const std::exception& e){return dynamic_cast<const JSTypeError*>(&e)?"TypeError":dynamic_cast<const JSRangeError*>(&e)?"RangeError":"Error";}
JSPropertyDescriptor data(JSValue v,bool writable=true,bool enumerable=true,bool configurable=true){JSPropertyDescriptor d;d.value=v;d.writable=writable;d.enumerable=enumerable;d.configurable=configurable;return d;}
JSPropertyDescriptor getter(JSValue f){JSPropertyDescriptor d;d.getter=f;d.enumerable=true;d.configurable=true;return d;}
JsonValue encode(JSValue v){
    if(v.isUndefined())return "$undefined";if(v.isCallable())return "$function";
    if(v.type()==JSValue::Type::Number){double n=v.number();if(std::isnan(n))return "$NaN";if(std::isinf(n))return n<0?"$-Infinity":"$Infinity";if(n==0&&std::signbit(n))return "$-0";return n;}
    if(v.type()==JSValue::Type::Array){JsonValue::Array a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(v.hasOwn(std::to_string(i))?encode(v.get(std::to_string(i))):JsonValue("$hole"));return a;}
    if(v.isObject()){JsonValue::Object o;for(const auto&k:v.keys())o[k]=encode(v.get(k));return o;}
    return *v.toJSON();
}
JSValue decode(JSValue v){
    if(v.strictEquals("$undefined"))return {};if(v.strictEquals("$NaN"))return std::numeric_limits<double>::quiet_NaN();
    if(v.strictEquals("$Infinity"))return std::numeric_limits<double>::infinity();if(v.strictEquals("$-Infinity"))return -std::numeric_limits<double>::infinity();if(v.strictEquals("$-0"))return -0.;return v;
}
JsonValue snapshot(JSValue o){JsonValue::Array entries;for(const auto&k:o.ownPropertyNames()){
    const auto d=*o.getOwnPropertyDescriptor(k);JsonValue::Object fields{{"key",k},{"enumerable",d.enumerable.value_or(false)},{"configurable",d.configurable.value_or(false)}};
    if(d.value){fields["value"]=encode(*d.value);fields["writable"]=d.writable.value_or(false);}else {fields["get"]=d.getter&&d.getter->isCallable();fields["set"]=d.setter&&d.setter->isCallable();}
    entries.emplace_back(fields);
}return entries;}
void effect(JSValue self,JSValue spec){
    if(spec.hasOwn("erase"))self.erase(spec.get("erase").toString());
    if(spec.hasOwn("setLength"))self.set("length",decode(spec.get("setLength")));
    if(spec.hasOwn("setKey"))self.set(spec.get("setKey").toString(),decode(spec.get("setValue")));
    if(spec.get("throw").toBoolean())throw JSTypeError("test callback");
}
JSValue converted(JSValue spec,JSValue self,JSValue log){
    if(!spec.isObject()||!spec.hasOwn("coerce"))return decode(spec);
    auto value=JSValue::object();value.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[2],c[0].get("coerce"));c[1].collectGarbage();effect(c[1],c[0]);return decode(c[0].get("value"));},{spec,self,log}));return value;
}
JsonValue arrayScenario(JSValue r){
    auto log=JSValue::array();const bool isArray=r.get("array").toBoolean();auto self=isArray?JSValue::array():JSValue::object();
    const auto entries=r.get("entries");for(const auto&k:entries.keys())self.set(k,decode(entries.get(k)));
    self.set("length",decode(r.get("length")));
    if(r.hasOwn("prototype")){auto p=JSValue::object(self.getPrototype());auto values=r.get("prototype");for(const auto&k:values.keys())p.set(k,decode(values.get(k)));self.setPrototype(p);}
    if(r.hasOwn("descriptors")){auto ds=r.get("descriptors");for(std::uint32_t i=0;i<ds.length();++i){const auto spec=ds.get(std::to_string(i));auto key=spec.get("key").toString();JSPropertyDescriptor d;
        if(spec.get("accessor").toBoolean()){
            d.getter=JSValue::function([](const JSValue&receiver,const Args&,const Args&c){push(c[1],list({"get",c[0].get("key")}));receiver.collectGarbage();effect(receiver,c[0]);return decode(c[0].get("value"));},{spec,log});
            if(spec.get("setter").toBoolean())d.setter=JSValue::function([](const JSValue&,const Args&a,const Args&c){push(c[1],list({"set",c[0].get("key"),a[0]}));return JSValue();},{spec,log});
        }else {d.value=decode(spec.get("value"));d.writable=!spec.hasOwn("writable")||spec.get("writable").toBoolean();}
        d.configurable=!spec.hasOwn("configurable")||spec.get("configurable").toBoolean();d.enumerable=!spec.hasOwn("enumerable")||spec.get("enumerable").toBoolean();self.defineProperty(key,d);
    }}
    if(r.hasOwn("constructor")){const auto spec=r.get("constructor");auto c=converted(spec,self,log);
        if(spec.isObject()&&spec.hasOwn("get")){self.defineProperty("constructor",getter(JSValue::function([](const JSValue&receiver,const Args&,const Args&c){push(c[1],"constructor");effect(receiver,c[0]);const auto value=c[0].get("get");return value.strictEquals("$object")?JSValue::object():decode(value);},{spec,log})));}
        else self.set("constructor",c.strictEquals("$object")?JSValue::object():c);
    }
    if(r.get("freezeLength").toBoolean()){JSPropertyDescriptor d;d.writable=false;self.defineProperty("length",d);}
    if(r.get("preventExtensions").toBoolean())self.preventExtensions();
    Args args;const auto aa=r.get("args");for(std::uint32_t i=0;i<aa.length();++i)args.push_back(converted(aa.get(std::to_string(i)),self,log));
    JSValue result;std::string failure;try{result=JSValue::array().get(r.get("method").toString()).call(self,args);}catch(const std::exception&e){failure=error(e);}
    return JsonValue::Object{{"error",failure},{"result",encode(result)},{"state",snapshot(self)},{"log",encode(log)}};
}
JSValue recordListener(const char* label,JSValue log){return JSValue::function([](const JSValue&receiver,const Args&a,const Args&c){push(c[0],list({c[1],a[0].get("target").strictEquals(receiver)}));receiver.collectGarbage();return JSValue();},{log,label});}
JsonValue eventScenario(JSValue r){
    const auto name=r.get("name").toString();auto self=EventDispatcher::runtimeObject(),log=JSValue::array(),event=object({{"type","x"}});JSValue result;std::string failure;
    if(name=="metadata"){
        JsonValue::Array names;for(const auto&k:self.getPrototype().ownPropertyNames())names.emplace_back(k);
        JsonValue::Array methods;for(const auto*k:{"addEventListener","hasEventListener","removeEventListener","dispatchEvent"})methods.emplace_back(JsonValue::Array{k,encode(self.get(k).get("name")),encode(self.get(k).get("length")),self.get(k).sameValue(EventDispatcher::runtimeObject().get(k)),self.hasOwn(k)});
        try{self.get("constructor").call();}catch(const std::exception&e){failure=error(e);}
        return JsonValue::Object{{"names",names},{"constructor",encode(self.get("constructor").get("name"))},{"prototypeIdentity",self.get("constructor").get("prototype").sameValue(self.getPrototype())},{"methods",methods},{"error",failure}};
    }
    auto a=recordListener("a",log),b=recordListener("b",log),c=recordListener("c",log);
    try{
        if(name=="basic"){
            push(log,call(self,"hasEventListener",{"x",a}));call(self,"removeEventListener",{"x",a});call(self,"addEventListener",{"x",a});call(self,"addEventListener",{"x",a});call(self,"addEventListener",{"x",b});push(log,call(self,"hasEventListener",{"x",a}));call(self,"dispatchEvent",{event});call(self,"removeEventListener",{"x",a});call(self,"dispatchEvent",{event});push(log,call(self,"hasEventListener",{"x",a}));
        }else if(name=="mutate"){
            a=JSValue::function([](const JSValue&receiver,const Args&a,const Args&c){push(c[0],list({"a",a[0].get("target").sameValue(receiver)}));call(receiver,"removeEventListener",{"x",c[1]});call(receiver,"addEventListener",{"x",c[2]});receiver.collectGarbage();return JSValue();},{log,b,c});
            call(self,"addEventListener",{"x",a});call(self,"addEventListener",{"x",b});call(self,"dispatchEvent",{event});call(self,"dispatchEvent",{event});
        }else if(name=="throw"){
            a=JSValue::function([](const JSValue&,const Args&,const Args&c)->JSValue{push(c[0],"throw");throw JSTypeError("listener");},{log});call(self,"addEventListener",{"x",a});call(self,"addEventListener",{"x",b});call(self,"dispatchEvent",{event});
        }else if(name=="reenter"){
            auto depth=object({{"value",0}});a=JSValue::function([](const JSValue&receiver,const Args&a,const Args&c){push(c[0],list({"a",a[0].get("target").sameValue(receiver)}));auto state=c[1];if(state.get("value").strictEquals(0)){state.set("value",1);call(receiver,"dispatchEvent",{a[0]});state.set("value",0);}return JSValue();},{log,depth});call(self,"addEventListener",{"x",a});call(self,"addEventListener",{"x",b});call(self,"dispatchEvent",{event});
        }else if(name=="callOverride"){
            auto listener=JSValue::object();auto replacement=JSValue::function([](const JSValue&receiver,const Args&a,const Args&c){push(c[0],list({"call",receiver.sameValue(c[1]),a[0].sameValue(c[2]),a[1].get("target").sameValue(a[0])}));receiver.collectGarbage();return JSValue();},{log,listener,self});
            listener.defineProperty("call",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"getCall");return c[1];},{log,replacement})));call(self,"addEventListener",{"x",listener});call(self,"dispatchEvent",{event});
        }else if(name=="early"){
            event.defineProperty("type",getter(JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("must not read");})));call(self,"dispatchEvent",{event});
        }else if(name=="readonlyTarget"){
            event.defineProperty("target",data(3,false));call(self,"addEventListener",{"x",a});call(self,"dispatchEvent",{event});
        }else if(name=="typeOrder"){
            auto key=JSValue::object();key.set("toString",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"type");return JSValue("x");},{log}));auto listeners=JSValue::object();self.defineProperty("_listeners",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"listeners");return c[1];},{log,listeners})));call(self,"addEventListener",{key,a});call(self,"hasEventListener",{key,a});call(self,"removeEventListener",{key,a});event.set("type",key);call(self,"dispatchEvent",{event});
        }else if(name=="sliceOverride"){
            auto length=JSValue::object();length.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"length");return JSValue(2);},{log}));auto copied=object({{"0",a},{"1",b},{"length",length}});auto listeners=list({a});listeners.defineProperty("slice",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"slice");return JSValue::function([](const JSValue&,const Args&a,const Args&c){push(c[0],list({"sliceCall",a[0]}));return c[1];},{c[0],c[1]});},{log,copied})));self.set("_listeners",object({{"x",listeners}}));call(self,"dispatchEvent",{event});
        }else if(name=="prototypeKey")call(self,"addEventListener",{r.get("key"),a});
        else throw JSTypeError("Unknown event scenario");
    }catch(const std::exception&e){failure=error(e);}
    auto target=event.get("target");return JsonValue::Object{{"error",failure},{"log",encode(log)},{"target",target.sameValue(self)?JsonValue("$self"):encode(target)}};
}
JsonValue functionScenario(JSValue r){
    auto log=JSValue::array();auto f=JSValue::function([](const JSValue&receiver,const Args&a,const Args&c){receiver.isObject()?receiver.collectGarbage():c[0].collectGarbage();auto values=JSValue::array();for(const auto&v:a)push(values,v);push(c[0],list({receiver,values}));return JSValue(17);},{log});
    f.set("call",17);auto method=JSValue::function([](const JSValue&,const Args&,const Args&){return JSValue();}).get("call");Args args;auto input=r.get("args");for(std::uint32_t i=0;i<input.length();++i)args.push_back(decode(input.get(std::to_string(i))));
    JSValue result;std::string failure;try{result=method.call(r.get("nonCallable").toBoolean()?JSValue::object():f,args);}catch(const std::exception&e){failure=error(e);}return JsonValue::Object{{"error",failure},{"result",encode(result)},{"log",encode(log)}};
}
}
int main(){const std::string input(std::istreambuf_iterator<char>(std::cin),{});auto requests=Animation_detail::parseJSON(input);JsonValue::Array out;for(std::uint32_t i=0;i<requests.length();++i){auto r=requests.get(std::to_string(i));try{out.push_back(r.get("kind").strictEquals("event")?eventScenario(r):r.get("kind").strictEquals("function")?functionScenario(r):arrayScenario(r));}catch(const std::exception&e){out.emplace_back(JsonValue::Object{{"outerError",error(e)}});}requests.collectGarbage();}std::cout<<JsonValue(out).dump()<<'\n';}

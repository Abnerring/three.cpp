#include "animation/AnimationMixer.h"
#include "animation/AnimationUtils.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
JSValue at(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
void push(JSValue a, JSValue value) { a.set(std::to_string(a.length()),value); }
JSValue arr(std::initializer_list<JSValue> values) { auto a=JSValue::array(); for(const auto& v:values)push(a,v);return a; }
JSValue call(JSValue o,const std::string& name,const Args& args={}) { return o.get(name).call(o,args); }
void record(JSValue state,JSValue entry) { if(state.get("enabled").toBoolean())push(state.get("trace"),entry); }
JSValue label(JSValue value) { if(value.isObject()&&value.has("label"))return value.get("label"); return value; }
void watch(JSValue object,const std::string& name,JSValue state,const std::vector<std::string>& keys) {
    auto storage=JSValue::object();
    for(const auto& key:keys) {
        storage.set(key,object.get(key));JSPropertyDescriptor d;d.configurable=d.enumerable=true;
        d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
            record(c[0],arr({"get",c[2],c[3]}));return c[1].get(c[3].string());
        },{state,storage,name,key});
        d.setter=JSValue::function([](const JSValue&,const Args& a,const Args& c)->JSValue {
            record(c[0],arr({"set",c[2],c[3],label(at(a,0))}));auto storage=c[1];storage.set(c[3].string(),at(a,0));return {};
        },{state,storage,name,key});object.defineProperty(key,d);
    }
}
JsonValue encode(JSValue v) {
    if(v.isUndefined())return "$undefined";
    if(v.type()==JSValue::Type::Number) { const auto n=v.number();if(std::isnan(n))return "$NaN";if(std::isinf(n))return n<0?"$-Infinity":"$Infinity";if(n==0&&std::signbit(n))return "$-0";return n; }
    if(!v.isObject())return *v.toJSON();
    if(v.isCallable())return "$function";
    if(v.hasOwn("label"))return v.get("label").toString();
    if(v.type()==JSValue::Type::Array){JsonValue::Array a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(v.hasOwn(std::to_string(i))?encode(v.get(std::to_string(i))):JsonValue("$hole"));return a;}
    JsonValue::Object o;for(const auto& key:v.keys())o[key]=encode(v.get(key));return o;
}
JSValue resolve(JSValue value,JSValue refs) {
    if(value.isObject()&&value.hasOwn("ref"))return refs.get(value.get("ref").string());
    if(value.isObject()&&value.hasOwn("number"))return value.get("number").toNumber();
    if(value.isObject()&&value.hasOwn("undefined"))return {};
    return value;
}
JSValue fixture(JSValue request,JSValue state) {
    auto refs=JSValue::object();
    for(int i=0;i<2;++i){auto root=JSValue::object();root.set("uuid","r"+std::to_string(i));root.set("label","r"+std::to_string(i));refs.set("r"+std::to_string(i),root);}
    auto mixer=AnimationMixer(refs.get("r0")).object();mixer.set("label","m");refs.set("m",mixer);
    for(int i=0;i<2;++i){auto clip=JSValue::object();clip.set("uuid","c"+std::to_string(i));clip.set("tracks",JSValue::array());clip.set("label","c"+std::to_string(i));refs.set("c"+std::to_string(i),clip);}
    for(int i=0;i<2;++i){
        auto binding=JSValue::object(),prop=JSValue::object();prop.set("rootNode",refs.get("r"+std::to_string(i)));prop.set("path",".value");
        binding.set("label","b"+std::to_string(i));binding.set("binding",prop);binding.set("referenceCount",i?1:2);binding.set("useCount",0);binding.set("_cacheIndex",nullptr);
        for(const auto* method:{"saveOriginalState","restoreOriginalState","apply"})binding.set(method,JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue {
            record(c[0],arr({"call",self.get("label"),c[1],at(a,0)}));return {};
        },{state,method}));refs.set("b"+std::to_string(i),binding);
    }
    for(int i=0;i<3;++i){auto action=JSValue::object();action.set("label","a"+std::to_string(i));action.set("_localRoot",i==1?refs.get("r1"):JSValue(nullptr));
        action.set("_clip",refs.get(i==2?"c1":"c0"));action.set("_propertyBindings",arr({refs.get(i==1?"b1":"b0")}));action.set("_interpolants",JSValue::array());
        action.set("_cacheIndex",nullptr);action.set("_byClipCacheIndex",nullptr);action.set("time",0);
        action.set("stop",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {record(c[0],arr({"call",self.get("label"),"stop"}));call(c[1],"_deactivateAction",{self});return self;},{state,mixer}));
        action.set("_update",JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue {auto row=arr({"call",self.get("label"),"_update"});for(const auto& v:a)push(row,v);record(c[0],row);return {};},{state}));
        refs.set("a"+std::to_string(i),action);
    }
    if(request.get("watch").toBoolean()) {
        auto keys=mixer.keys();keys.erase(std::remove(keys.begin(),keys.end(),"label"),keys.end());watch(mixer,"m",state,keys);
        for(const auto* name:{"r0","r1","c0","c1"})watch(refs.get(name),name,state,{"uuid"});
        for(const auto* name:{"a0","a1","a2"})watch(refs.get(name),name,state,{"_localRoot","_clip","_propertyBindings","_interpolants","_cacheIndex","_byClipCacheIndex","time","stop","_update"});
        for(const auto* name:{"b0","b1"})watch(refs.get(name),name,state,{"binding","referenceCount","useCount","_cacheIndex","saveOriginalState","restoreOriginalState","apply"});
        for(const auto* name:{"_isActiveAction","_bindAction","_addInactiveAction","_lendAction","_takeBackAction","_lendBinding","_takeBackBinding","_removeInactiveBinding","_removeInactiveBindingsForAction","_deactivateAction","_removeInactiveAction","existingAction","update"})watch(mixer,"m",state,{name});
    }
    return refs;
}
JsonValue snapshot(JSValue refs) {
    auto state=JSValue::object(),m=refs.get("m");
    for(const auto* key:{"_actions","_nActiveActions","_actionsByClip","_bindings","_nActiveBindings","_bindingsByRootAndName","time","timeScale","_accuIndex","stats"})state.set(key,m.get(key));
    auto objects=JSValue::object();
    for(const auto* name:{"a0","a1","a2","b0","b1"}){auto o=refs.get(name),item=JSValue::object();for(const auto* key:{"_cacheIndex","_byClipCacheIndex","time","referenceCount","useCount"})item.set(key,o.get(key));objects.set(name,item);}state.set("objects",objects);return encode(state);
}
JsonValue run(JSValue request) {
    auto state=JSValue::object();state.set("enabled",false);state.set("trace",JSValue::array());auto refs=fixture(request,state);JsonValue::Array output;
    const auto steps=request.get("steps");for(std::uint32_t i=0;i<steps.length();++i){const auto step=steps.get(std::to_string(i));state.set("trace",JSValue::array());state.set("enabled",true);JSValue result;std::string error;
        try {const auto target=refs.get(step.hasOwn("target")?step.get("target").string():"m");if(step.hasOwn("set")){auto o=target;o.set(step.get("set").string(),resolve(step.get("value"),refs));}
            else {Args args;const auto input=step.get("args");if(!input.isUndefined())for(std::uint32_t j=0;j<input.length();++j)args.push_back(resolve(input.get(std::to_string(j)),refs));result=call(target,step.get("method").string(),args);}}
        catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}catch(const std::exception& e){error=std::string("C++:")+e.what();}
        state.set("enabled",false);JsonValue::Object row;row["error"]=error;row["result"]=encode(result);row["trace"]=encode(state.get("trace"));row["state"]=snapshot(refs);output.push_back(row);refs.collectGarbage();
    }return output;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(Animation_detail::parseJSON(line)).dump()<<'\n';}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}

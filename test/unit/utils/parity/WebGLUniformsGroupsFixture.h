#pragma once
#include "renderers/webgl/WebGLUniformsGroups.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace GroupsFixture {
using Args = std::vector<JSValue>; using A = JsonValue::Array; using O = JsonValue::Object;
inline void prop(JSValue object, const std::string& key, JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = d.enumerable = d.configurable = true; object.defineProperty(key,d);
}
inline JSValue arr(Args values = {}) {
    auto result = JSValue::array(); for(std::size_t i=0;i<values.size();++i) prop(result,std::to_string(i),values[i]); return result;
}
inline JSValue invoke(const JSValue& object,const char* name,Args args={}) {return object.get(name).call(object,args);}
inline void push(JSValue object,JSValue value){invoke(object,"push",{value});}
inline JsonValue encode(const JSValue& value) {
    if(value.isUndefined()) return "undefined"; if(value.isNull()) return nullptr;
    if(value.type()==JSValue::Type::Boolean) return value.toBoolean();
    if(value.type()==JSValue::Type::String) return value.string();
    if(value.type()==JSValue::Type::Number) {
        const auto n=value.number(); if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";
        if(n==0&&std::signbit(n))return "-0";return n;
    }
    if(value.isCallable())return O{{"function",value.get("name").toString()}};
    if(value.hasOwn("tag"))return O{{"ref",value.get("tag").toString()}};
    if(value.isHostObject("three.AnimationArray")) {
        A values;for(double i=0;i<value.get("length").toNumber();++i)values.push_back(encode(value.get(JSValue(i))));
        return O{{"typed",value.get("constructor").get("name").toString()},{"values",values}};
    }
    if(value.type()==JSValue::Type::Array) {A values;for(unsigned i=0;i<value.length();++i)values.push_back(encode(value.get(JSValue(i))));return values;}
    O result;for(const auto& key:value.keys()){const auto d=value.getOwnPropertyDescriptor(key);result[key]=d&&d->value?encode(*d->value):JsonValue("accessor");}return result;
}
inline JSValue snap(const JSValue& value){return JSValue::fromJSON(encode(value));}
inline void log(JSValue state,Args row){push(state.get("log"),arr(std::move(row)));}
template<class F> JsonValue outcome(F action) {try{return encode(action());}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const std::exception& e){return O{{"error","Error"},{"message",e.what()}};}}
inline JSValue numbers(unsigned n,double start=.25){auto a=JSValue::array();for(unsigned i=0;i<n;++i)prop(a,std::to_string(i),start+i);return a;}
inline JSValue value(JSValue state,const char* flag,unsigned n,double start=.25);
inline JSValue valueMethod(const JSValue& self,const Args& args,const Args& c) {
    auto state=c[0];const auto name=c[1].string();log(state,{name});
    if(state.get("throw").strictEquals(name))throw JSRangeError(name);
    const auto values=self.get("values");
    if(name=="clone") {auto result=value(state,self.get("flag").toString().c_str(),values.length());result.set("values",invoke(values,"slice"));result.set("elements",result.get("values"));return result;}
    if(name=="equals") {
        if(!state.get("equalsReturn").isUndefined())return state.get("equalsReturn");
        const auto other=args[0].get("values");if(!values.get("length").strictEquals(other.get("length")))return false;
        for(unsigned i=0;i<values.length();++i)if(!values.get(JSValue(i)).strictEquals(other.get(JSValue(i))))return false;return true;
    }
    if(name=="copy") {auto writable=self;writable.set("values",invoke(args[0].get("values"),"slice"));writable.set("elements",writable.get("values"));return self;}
    auto data=args[0];const auto offset=args[1].toNumber();for(unsigned i=0;i<values.length();++i)data.set(JSValue(offset+i),values.get(JSValue(i)));return data;
}
inline JSValue value(JSValue state,const char* flag,unsigned n,double start) {
    auto result=JSValue::object();prop(result,"flag",flag);prop(result,flag,true);prop(result,"values",numbers(n,start));prop(result,"elements",result.get("values"));
    for(const char* name:{"clone","equals","copy","toArray"})prop(result,name,JSValue::function(valueMethod,{state,name},name,name==std::string("clone")?0:name==std::string("toArray")?2:1));
    return result;
}
inline JSValue uniform(JSValue value,const JSValue& type={}) {auto u=JSValue::object();prop(u,"value",value);prop(u,"type",type);return u;}
inline JSValue command(const JSValue& self,const Args& args,const Args& c) {
    auto state=c[0];const auto name=c[1].string();Args row{name,self.sameValue(state.get(c[2].string()))};
    for(const auto& a:args)row.push_back(snap(a));log(state,std::move(row));
    if(state.get("throw").strictEquals(name))throw JSRangeError(name);
    if(name=="createBuffer") {
        if(state.get("bufferReturnEnabled").toBoolean())return state.get("bufferReturn");
        auto b=JSValue::object();const auto n=state.get("next").toNumber();state.set("next",n+1);prop(b,"tag","buffer:"+JSValue(n).toString());return b;
    }
    if(state.get("reenter").strictEquals(name)&&!state.get("acted").toBoolean()) {
        state.set("acted",true);invoke(state.get("manager"),state.get("reenterAction").toString().c_str(),{state.get("group"),state.get("program")});
    }
    return {};
}
inline JSValue eventMethod(const JSValue& self,const Args& args,const Args& c) {
    auto state=c[0];const auto name=c[1].string();log(state,{name,self.sameValue(state.get("group")),args[0],snap(args[1])});
    if(state.get("throw").strictEquals(name))throw JSRangeError(name);
    auto listeners=state.get("listeners");
    if(name=="addEventListener") {for(unsigned i=0;i<listeners.length();++i)if(listeners.get(JSValue(i)).sameValue(args[1]))return {};push(listeners,args[1]);}
    else {for(unsigned i=0;i<listeners.length();++i)if(listeners.get(JSValue(i)).sameValue(args[1])){invoke(listeners,"splice",{i,1});break;}}
    return {};
}
struct Fixture {
    JSValue state=JSValue::object(),gl=JSValue::object(),info=JSValue::object(),binding=JSValue::object(),group=JSValue::object(),program=JSValue::object(),manager;
    Fixture() {
        prop(state,"log",arr());prop(state,"listeners",arr());prop(state,"next",0);prop(state,"throw","");prop(state,"acted",false);
        prop(state,"reenter","");prop(state,"reenterAction","dispose");prop(state,"gl",gl);prop(state,"binding",binding);prop(state,"group",group);prop(state,"program",program);
        prop(gl,"UNIFORM_BUFFER",35345);auto render=JSValue::object();prop(render,"frame",0);prop(info,"render",render);
        prop(group,"tag","group");prop(group,"id",1);prop(group,"usage",35044);prop(group,"uniforms",arr());auto p=JSValue::object();prop(p,"tag","program");prop(program,"program",p);
        for(const char* name:{"createBuffer","bindBuffer","bufferData","bufferSubData","deleteBuffer"})prop(gl,name,JSValue::function(command,{state,name,"gl"},name));
        for(const char* name:{"uniformBlockBinding","updateUBOMapping"})prop(binding,name,JSValue::function(command,{state,name,"binding"},name));
        for(const char* name:{"addEventListener","removeEventListener"})prop(group,name,JSValue::function(eventMethod,{state,name},name,2));
        manager=WebGLUniformsGroups(gl,info,{},binding).object();prop(state,"manager",manager);
        ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args row{"warning"};for(const auto& arg:args)row.push_back(snap(arg));log(c[0],row);return {};},{state}));
    }
    ~Fixture(){ThreeUtils::setConsoleFunction({});}
    JSValue update(){return invoke(manager,"update",{group,program});}
    JSValue bind(){return invoke(manager,"bind",{group,program});}
    void frame(JSValue value){info.get("render").set("frame",value);}
    JSValue dispatch(){auto event=JSValue::object();prop(event,"target",group);auto listeners=invoke(state.get("listeners"),"slice");for(unsigned i=0;i<listeners.length();++i)listeners.get(JSValue(i)).call(group,{event});return {};}
    JsonValue snapshot() {A uniforms;auto list=group.get("uniforms");for(unsigned i=0;i<list.length();++i){if(!list.has(JSValue(i))){uniforms.emplace_back("undefined");continue;}auto item=list.get(JSValue(i));auto items=item.type()==JSValue::Type::Array?item:arr({item});A entries;for(unsigned j=0;j<items.length();++j){if(!items.has(JSValue(j))){entries.emplace_back("undefined");continue;}auto u=items.get(JSValue(j));entries.push_back(O{{"offset",encode(u.get("__offset"))},{"data",encode(u.get("__data"))}});}uniforms.push_back(entries);}return O{{"size",encode(group.get("__size"))},{"cache",encode(group.get("__cache"))},{"uniforms",uniforms},{"listeners",encode(state.get("listeners"))},{"trace",encode(state.get("log"))}};}
};
inline JsonValue scalarCase(unsigned mode) {Fixture f;const Args values{1,true,false,-0.,std::nan(""),std::numeric_limits<double>::infinity(),std::numeric_limits<double>::infinity()*-1};auto u=uniform(values[mode%values.size()],mode<7?JSValue():mode<14?JSValue("int"):JSValue("uint"));f.group.set("uniforms",arr({u}));A results;results.push_back(outcome([&]{return f.bind();}));for(unsigned i=0;i<3;++i){f.frame(i);results.push_back(outcome([&]{return f.update();}));results.push_back(outcome([&]{return f.update();}));}results.push_back(outcome([&]{return f.bind();}));return O{{"kind","scalar"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue packedCase(unsigned mode) {Fixture f;const char* flags[]{"isVector2","isVector3","isColor","isVector4","isMatrix3","isMatrix4"};const unsigned widths[]{2,3,3,4,9,16};Args us{uniform(1)};for(unsigned i=0;i<6;++i){us.push_back(uniform(value(f.state,flags[i],widths[i]),mode==1?JSValue("int"):mode==2?JSValue("uint"):JSValue()));us.push_back(uniform(true));}f.group.set("uniforms",arr(us));A results;for(unsigned i=0;i<3;++i){f.frame(i);if(i==2)us[3].get("value").get("values").set("0",99);results.push_back(outcome([&]{return f.update();}));}return O{{"kind","packed"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue arrayCase(unsigned mode) {Fixture f;JSValue v=arr({1,2,3});if(mode==1)v=arr({value(f.state,"isVector2",2),value(f.state,"isVector2",2,5)});if(mode==2)v=arr({value(f.state,"isVector3",3),value(f.state,"isVector3",3,5)});if(mode==3)v=arr({value(f.state,"isMatrix3",9),value(f.state,"isMatrix3",9,20)});if(mode==4)v=arr();auto u=uniform(v);if(mode==5)f.group.set("uniforms",arr({arr({uniform(1),uniform(2),uniform(3)}),uniform(4)}));else if(mode==6)f.group.set("uniforms",arr({arr(),u}));else f.group.set("uniforms",arr({uniform(9),u,uniform(4)}));A results;for(unsigned i=0;i<4;++i){f.frame(i);if(i==1)v.set("length",1);if(i==2)v.set("length",0);if(i==3)push(v,7);results.push_back(outcome([&]{return f.update();}));}return O{{"kind","array"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue typedCase(unsigned kind,unsigned mode) {Fixture f;auto v=NativeArray_detail::constructor(kind).construct({numbers(mode==3?0:mode==2?3:4)});if(mode==1)prop(v,"isVector3",true);auto u=uniform(v);f.group.set("uniforms",arr({uniform(1),u,uniform(2)}));A results;for(unsigned i=0;i<3;++i){f.frame(i);if(i==2)v.set("0",77);results.push_back(outcome([&]{return f.update();}));}return O{{"kind","typed"},{"type",kind},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue failureCase(unsigned mode) {Fixture f;const char* names[]{"createBuffer","bindBuffer","bufferData","addEventListener","updateUBOMapping","clone","toArray","bufferSubData","deleteBuffer","removeEventListener","copy","equals"};auto v=value(f.state,"isVector3",3);f.group.set("uniforms",arr({uniform(v)}));A results;if(mode>=8)results.push_back(outcome([&]{return f.update();}));if(mode>=10){f.frame(1);v.get("values").set("0",88);}f.state.set("throw",names[mode]);results.push_back(outcome([&]{return mode<8||mode>=10?f.update():f.dispatch();}));const auto failed=f.snapshot();f.state.set("throw","");results.push_back(outcome([&]{return f.update();}));return O{{"kind","failure"},{"mode",mode},{"results",results},{"failed",failed},{"snapshot",f.snapshot()}};}
inline JsonValue lifecycleCase(unsigned mode) {Fixture f;auto u=uniform(1);f.group.set("uniforms",arr({u}));A results;results.push_back(outcome([&]{return f.update();}));if(mode==0)results.push_back(outcome([&]{return f.dispatch();}));if(mode==1)results.push_back(outcome([&]{return invoke(f.manager,"dispose");}));if(mode==2){f.group.set("id",2);results.push_back(outcome([&]{return f.update();}));results.push_back(outcome([&]{return invoke(f.manager,"dispose");}));}if(mode==3){f.group.set("id","__proto__");results.push_back(outcome([&]{return f.update();}));}if(mode==4){f.state.set("reenter","bufferSubData");f.state.set("acted",false);f.frame(1);u.set("value",2);results.push_back(outcome([&]{return f.update();}));}results.push_back(outcome([&]{return f.bind();}));results.push_back(outcome([&]{return f.update();}));results.push_back(outcome([&]{return f.dispatch();}));results.push_back(outcome([&]{return invoke(f.manager,"dispose");}));return O{{"kind","lifecycle"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline void watch(Fixture& f,JSValue object,const char* property,const char* label,bool fail=false) {auto saved=object.get(property);JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[0],{"get",c[1],c[2],self.sameValue(c[3])});if(c[5].toBoolean())throw JSRangeError("getter");return c[4];},{f.state,label,property,object,saved,fail});d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[0],{"set",c[1],c[2],self.sameValue(c[3]),snap(a[0])});return {};},{f.state,label,property,object});object.defineProperty(property,d);}
inline JsonValue observedCase(unsigned mode) {Fixture f;auto v=value(f.state,"isMatrix3",9),u=uniform(v);f.group.set("uniforms",arr({u}));if(mode==0)watch(f,u,"value","uniform");if(mode==1)watch(f,v,"elements","matrix");if(mode==2)watch(f,v,"isVector2","value");if(mode==3)watch(f,f.group,"id","group");if(mode==4)watch(f,f.gl,"UNIFORM_BUFFER","gl");if(mode==5)watch(f,f.gl,"bufferSubData","gl");if(mode==6)watch(f,f.group,"uniforms","group");if(mode==7)watch(f,u,"type","uniform");if(mode==8)watch(f,f.program,"program","program");if(mode==9)watch(f,f.info.get("render"),"frame","render");if(mode==10)watch(f,u,"value","uniform",true);if(mode==11){watch(f,v,"clone","value");watch(f,v,"toArray","value");}A results;results.push_back(outcome([&]{return f.update();}));f.frame(1);results.push_back(outcome([&]{return f.update();}));results.push_back(outcome([&]{return f.bind();}));return O{{"kind","observed"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue unsupportedCase(unsigned mode) {Fixture f;JSValue v;if(mode==0)v=nullptr;if(mode==1)v=JSValue();if(mode==2)v="x";if(mode>=3){v=value(f.state,mode==3?"isTexture":"unsupported",0);if(mode==5)f.state.set("equalsReturn",0);if(mode==6)f.state.set("equalsReturn",false);}f.group.set("uniforms",arr({uniform(v)}));A results;for(unsigned i=0;i<2;++i){f.frame(i);results.push_back(outcome([&]{return f.update();}));}return O{{"kind","unsupported"},{"mode",mode},{"results",results},{"snapshot",f.snapshot()}};}
inline JsonValue edgeCase(unsigned mode) {Fixture f;auto v=value(f.state,"isVector3",3),u=uniform(v);f.group.set("uniforms",arr({u}));
 if(mode==0)f.group.set("uniforms",arr());if(mode==1)u.set("value",arr());if(mode==2)f.group.set("uniforms",arr({u,u}));if(mode==4)f.frame(JSValue());if(mode==5)f.frame(std::nan(""));
 if(mode==7)f.state.set("equalsReturn",0);if(mode==8)v.set("clone",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"clone"));
 if(mode>=9&&mode<=11){f.state.set("bufferReturnEnabled",true);f.state.set("bufferReturn",mode==9?JSValue(nullptr):mode==10?JSValue(false):JSValue());}
 if(mode==12)f.group.set("id",JSValue());if(mode==13)f.group.set("id","__proto__");if(mode==14)f.group.preventExtensions();if(mode==15)watch(f,u,"__data","uniform");
 if(mode==16){auto hole=JSValue::array(1);f.group.set("uniforms",hole);}if(mode==17)f.group.set("uniforms",arr({arr({u}),uniform(2)}));
 if(mode==18)u.set("value",arr({1,v}));if(mode==20)f.gl.set("bufferSubData",9);if(mode==21)f.gl.set("UNIFORM_BUFFER",nullptr);
 if(mode==22)f.state.set("reenter","bufferData");
 if(mode==25||mode==27){v=NativeArray_detail::constructor(6).construct({numbers(4)});u.set("value",v);watch(f,v,"byteLength","typed");if(mode==27){watch(f,v,"slice","typed");watch(f,v,"subarray","typed");}}
 A results;results.push_back(outcome([&]{return mode==19?invoke(f.manager,"update",{f.group,JSValue()}):f.update();}));
 if(mode==3)push(f.group.get("uniforms"),uniform(4));if(mode==6)u.set("type","uint");if(mode==18)u.set("value",arr({v,1}));
 if(mode==23)watch(f,f.group,"id","group");if(mode==24){v.get("values").set("0",99);watch(f,v,"clone","value");watch(f,v,"equals","value");watch(f,v,"copy","value");}
 if(mode==26){v=NativeArray_detail::constructor(6).construct({numbers(4)});u.set("value",v);watch(f,u.get("__data"),"set","data");watch(f,v,"subarray","typed");watch(f,v,"slice","typed");}
 if(mode!=4&&mode!=5)f.frame(1);results.push_back(outcome([&]{return f.update();}));results.push_back(outcome([&]{return f.update();}));
 auto snapshot=outcome([&]{return JSValue::fromJSON(f.snapshot());});return O{{"kind","edge"},{"mode",mode},{"results",results},{"snapshot",snapshot}};}
inline JsonValue metadata(const JSValue& fn) {A names;for(const auto& name:fn.ownPropertyNames())names.emplace_back(name);O out{{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"own",names},{"constructible",fn.isConstructible()},{"source",encode(invoke(fn,"toString"))}};if(const auto d=fn.getOwnPropertyDescriptor("prototype"))out["prototypeFlags"]=A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)};return out;}
inline JsonValue metadataCase() {Fixture f;auto ctor=nativeWebGLUniformsGroupsConstructor();O out{{"kind","metadata"},{"constructor",metadata(ctor)},{"identity",ctor.sameValue(nativeWebGLUniformsGroupsConstructor())}};for(const char* name:{"bind","update","dispose"})out[name]=metadata(f.manager.get(name));auto other=ctor.construct({f.gl,f.info,JSValue(),f.binding});out["constructResult"]=encode(other);out["methodConstruct"]=outcome([&]{return f.manager.get("dispose").construct();});return out;}
inline JsonValue identityCase(unsigned mode) {Fixture f;auto v=mode==0?numbers(3):mode==1?NativeArray_detail::constructor(6).construct({numbers(4)}):mode==2?value(f.state,"isMatrix3",9):arr({value(f.state,"isMatrix4",16),value(f.state,"isMatrix4",16,20)});auto u=uniform(v);f.group.set("uniforms",arr({u}));auto retained=arr();const auto original=f.gl.get("bufferSubData");f.gl.set("bufferSubData",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{push(c[0],args[2]);return c[1].call(self,args);},{retained,original},"bufferSubData",3));A results;results.push_back(outcome([&]{return f.update();}));const auto cache=f.group.get("__cache"),cached=cache.get("0_0"),data=u.get("__data");const auto before=encode(cached);if(mode==0||mode==1)v.set("0",99);if(mode==2)v.get("values").set("0",99);if(mode==3)v.get("0").get("values").set("0",99);f.frame(1);results.push_back(outcome([&]{return f.update();}));O out{{"kind","identity"},{"mode",mode},{"results",results},{"cacheSame",cache.sameValue(f.group.get("__cache"))},{"cachedSame",cached.sameValue(cache.get("0_0"))},{"dataSame",data.sameValue(u.get("__data"))},{"cachedAliasedValue",cached.sameValue(v)},{"retainedSame",retained.get("0").sameValue(data)&&retained.get("1").sameValue(data)},{"cacheBefore",before},{"cacheAfter",encode(cached)},{"snapshot",f.snapshot()}};if(mode==1)out["bufferAliased"]=cached.get("buffer").sameValue(v.get("buffer"));if(mode==2)out["elementsAliased"]=cached.get("elements").sameValue(v.get("elements"));if(mode==3){out["firstAliased"]=cached.get("0").sameValue(v.get("0"));out["elementsAliased"]=cached.get("0").get("elements").sameValue(v.get("0").get("elements"));}return out;}
}

#include "animation/AnimationUtils.h"
#include "core/NativeArray.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSWeakMap.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/StackTrace.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)out.set(std::to_string(out.length()),value);return out;}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
void event(const JSValue& state,const JSValue& value){call(state.get("events"),"push",{value});}
void property(JSValue object,const std::string& name,const JSValue& getter){JSPropertyDescriptor d;d.getter=getter;d.enumerable=d.configurable=true;object.defineProperty(name,d);}
JSValue decode(const JSValue& spec,const JSValue& state){
    if(!spec.isObject())return spec;
    if(spec.type()==JSValue::Type::Array){auto out=JSValue::array(spec.length());for(unsigned i=0;i<spec.length();++i)out.set(std::to_string(i),decode(spec.get(std::to_string(i)),state));return out;}
    const auto tag=spec.get("tag");
    if(tag.strictEquals("undefined"))return {};
    if(tag.strictEquals("nan"))return std::numeric_limits<double>::quiet_NaN();
    if(tag.strictEquals("infinity"))return std::numeric_limits<double>::infinity();
    if(tag.strictEquals("negative-zero"))return -0.0;
    if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("buffer")||tag.strictEquals("typed")){
        const auto values=decode(spec.get("values"),state);auto typed=NativeArray_detail::constructor(1).construct({values});return tag.strictEquals("buffer")?typed.get("buffer"):typed;
    }
    if(tag.strictEquals("coerce")||tag.strictEquals("stringCoerce")){
        auto out=JSValue::object(),count=JSValue::object();count.set("n",0);
        const auto key=tag.strictEquals("coerce")?"valueOf":"toString";
        property(out,key,JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
            event(captures[0],"get:"+captures[1].get("name").toString());
            return JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
                auto count=captures[2];const auto index=count.get("n").number();count.set("n",index+1);
                event(captures[0],"call:"+captures[1].get("name").toString()+":"+JSValue(index).toString());
                const auto values=captures[1].get("values");const auto size=values.length();return decode(values.get(std::to_string(static_cast<unsigned>(std::min(index,double(size-1))))),captures[0]);
            },captures);
        },{state,spec,count}));return out;
    }
    if(tag.strictEquals("flags")||tag.strictEquals("texture")){
        auto out=JSValue::object(),count=JSValue::object();count.set("n",0);
        const auto source=spec.get("values");for(const auto& key:source.ownPropertyNames()){
            property(out,key,JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
                const auto key=captures[3].string();event(captures[0],"get:"+key);const auto value=captures[1].get("values").get(key);
                if(key=="type"&&value.type()==JSValue::Type::Array){auto count=captures[2];const auto n=count.get("n").number();count.set("n",n+1);return decode(value.get(std::to_string(static_cast<unsigned>(std::min(n,double(value.length()-1))))),captures[0]);}
                return decode(value,captures[0]);
            },{state,spec,count,key}));
        }return out;
    }
    auto out=JSValue::object();for(const auto& key:spec.ownPropertyNames())out.set(key,decode(spec.get(key),state));return out;
}
JSValue encode(const JSValue& value){
    if(value.isUndefined())return "#undefined";if(value.isSymbol())return "#symbol";
    if(value.isCallable())return value.get("name");
    if(value.type()==JSValue::Type::Number){if(std::isnan(value.number()))return "#NaN";if(std::isinf(value.number()))return value.number()>0?"#Infinity":"#-Infinity";if(value.number()==0&&std::signbit(value.number()))return "#-0";}
    if(value.isHostObject("three.ArrayBuffer")){const auto bytes=NativeArray_detail::constructor(1).construct({value});auto out=JSValue::array();for(double i=0;i<bytes.get("length").number();++i)call(out,"push",{bytes.get(JSValue(i).toString())});return out;}
    if(value.isObject()){
        const auto type=nativeNodeUtils().get("getValueType").call({}, {value});
        if(type.strictEquals("vec2")||type.strictEquals("vec3")||type.strictEquals("vec4")||type.strictEquals("color")){
            auto out=JSValue::object();out.set("type",type);auto components=JSValue::array();const auto* keys=type.strictEquals("color")?"rgb":type.strictEquals("vec2")?"xy":type.strictEquals("vec3")?"xyz":"xyzw";for(const auto* p=keys;*p;++p)call(components,"push",{encode(value.get(std::string(1,*p)))});out.set("values",components);return out;
        }
        if(type.strictEquals("mat2")||type.strictEquals("mat3")||type.strictEquals("mat4")){auto out=JSValue::object();out.set("type",type);auto elements=value.get("elements"),values=JSValue::array();for(unsigned i=0;i<elements.length();++i)call(values,"push",{encode(elements.get(std::to_string(i)))});out.set("values",values);return out;}
        return "#object";
    }return value;
}
JSValue run(const JSValue& request){
    auto state=JSValue::object();state.set("events",array());auto result=JSValue::object();const auto kind=request.get("kind");
    if(kind.strictEquals("metadata")){
        auto rows=JSValue::array();const auto module=nativeNodeUtils();for(const auto& key:module.ownPropertyNames()){
            const auto fn=module.get(key);auto row=JSValue::object();row.set("name",fn.get("name"));row.set("length",fn.get("length"));row.set("prototype",fn.hasOwn("prototype"));row.set("constructible",fn.isConstructible());call(rows,"push",{row});
        }return rows;
    }
    if(kind.strictEquals("cache")){
        const auto fn=nativeNodeUtils().get("getDataFromObject"),key=JSValue::object(),other=JSValue::object();auto data=fn.call({}, {key});
        auto rows=array({data.strictEquals(fn.call({}, {key})),!data.strictEquals(fn.call({}, {other})),data.ownPropertyNames().empty()});data.set("cycle",key);JSWeakValue weakKey(key),weakData(data);call(rows,"push",{weakKey.expired(),weakData.expired()});return rows;
    }
    if(kind.strictEquals("mapProtocol")){
        auto module=nativeNodeUtils();const auto prototype=nativeMapConstructor().get("prototype");const auto original=*prototype.getOwnPropertyDescriptor("get");
        property(prototype,"get",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
            event(captures[0],"get:get");return JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"call:get:"+args[0].toString());return "custom";},captures);
        },{state}));
        const auto a=module.get("getTypeFromLength").call({}, {3});auto texture=JSValue::object();texture.set("format",0);texture.set("type",0);const auto b=module.get("getTextureType").call({}, {texture});auto mutablePrototype=prototype;mutablePrototype.defineProperty("get",original);
        result.set("value",array({a,b}));result.set("events",state.get("events"));return result;
    }
    if(kind.strictEquals("weakMapProtocol")){
        auto module=nativeNodeUtils();const auto prototype=nativeWeakMapConstructor().get("prototype");const auto originalGet=*prototype.getOwnPropertyDescriptor("get"),originalSet=*prototype.getOwnPropertyDescriptor("set");
        auto count=JSValue::object();count.set("n",0);
        property(prototype,"get",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
            event(captures[0],"get:get");return JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{auto count=captures[1];const auto n=count.get("n").number();count.set("n",n+1);event(captures[0],"call:get");return n==0?JSValue{}:JSValue(false);},captures);
        },{state,count}));
        property(prototype,"set",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
            event(captures[0],"get:set");return JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue{event(captures[0],"call:set");return captures[1].call(self,args);},captures);
        },{state,*originalSet.value}));
        const auto fn=module.get("getDataFromObject"),key=JSValue::object();const auto a=fn.call({}, {key}),b=fn.call({}, {key});auto mutablePrototype=prototype;mutablePrototype.defineProperty("get",originalGet);mutablePrototype.defineProperty("set",originalSet);
        result.set("value",array({encode(a),encode(b)}));result.set("events",state.get("events"));return result;
    }
    if(kind.strictEquals("hashMethod")){
        const auto prototype=nativeStringPrototype();const auto original=*prototype.getOwnPropertyDescriptor("charCodeAt");
        auto counter=JSValue::object();counter.set("tag","coerce");counter.set("name","char");counter.set("values",array({7,8,9,10,11,12}));const auto number=decode(counter,state);
        JSPropertyDescriptor d;d.value=JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"char:"+args[0].toString());return captures[1];},{state,number});d.writable=d.configurable=true;auto mutablePrototype=prototype;mutablePrototype.defineProperty("charCodeAt",d);
        const auto value=nativeNodeUtils().get("hashString").call({}, {"A\xF0\x9F\x98\x80"});mutablePrototype.defineProperty("charCodeAt",original);result.set("value",value);result.set("events",state.get("events"));return result;
    }
    if(kind.strictEquals("stack")){
        const auto object=nativeStackTraceConstructor().construct({request.get("text")});result.set("stack",object.get("stack"));result.set("location",call(object,"getLocation"));result.set("error",encode(call(object,"getError",{decode(request.get("message"),state)})));return result;
    }
    if(kind.strictEquals("stackProtocol")){
        auto object=nativeStackTraceConstructor().construct({"Error"});object.set("stack",decode(request.get("frames"),state));result.set("location",call(object,"getLocation"));result.set("error",encode(call(object,"getError",{decode(request.get("message"),state)})));result.set("events",state.get("events"));return result;
    }
    if(kind.strictEquals("hashMutation")){
        auto values=array({0,99}),number=JSValue::object();auto counter=JSValue::object();counter.set("n",0);
        number.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{
            auto count=captures[2],values=captures[1];const auto n=count.get("n").number();count.set("n",n+1);event(captures[0],"valueOf:"+JSValue(n).toString());if(n==0)values.set("length",1);return n+7;
        },{state,values,counter}));values.set("0",number);result.set("value",encode(nativeNodeUtils().get("hashArray").call({}, {values})));result.set("events",state.get("events"));return result;
    }
    const auto method=request.get("method").string();Args args;const auto specs=request.get("args");for(unsigned i=0;i<specs.length();++i)args.push_back(decode(specs.get(std::to_string(i)),state));
    try{result.set("value",encode(nativeNodeUtils().get(method).call({},args)));}
    catch(const JSTypeError&){result.set("error","TypeError");}
    catch(const NodeUtilsUnsupported&){result.set("error","Unsupported");}
    catch(const std::exception&){result.set("error","Error");}
    result.set("events",state.get("events"));return result;
}
}
int main(int argc,char** argv){try{
    if(argc>1&&std::string(argv[1])=="array-buffer-intrinsic"){
        const auto constructor=nativeArrayBufferConstructor();auto prototype=constructor.get("prototype");prototype.set("constructor",false);
        const auto buffer=nativeArrayBuffer(ArrayBuffer(0)),fake=JSValue::object(prototype);const auto fn=nativeNodeUtils().get("getValueType");std::cout<<array({fn.call({}, {buffer}),fn.call({}, {fake})}).toJSON()->dump();return 0;
    }
    if(argc>1&&std::string(argv[1])=="gc"){
        auto module=nativeNodeUtils(),rows=JSValue::array();
        for(const auto* mode:{"cycle","retain-key","retain-data","retain-data-cycle"}){
            JSWeakValue keyRef,dataRef;JSValue retained;
            {auto key=JSValue::object(),data=module.get("getDataFromObject").call({}, {key});keyRef=JSWeakValue(key);dataRef=JSWeakValue(data);
                if(std::string(mode)=="cycle"||std::string(mode)=="retain-data-cycle")data.set("key",key);
                if(std::string(mode)=="retain-key")retained=key;
                if(std::string(mode)=="retain-data"||std::string(mode)=="retain-data-cycle")retained=data;
            }module.collectGarbage();module.collectGarbage();call(rows,"push",{array({mode,!keyRef.expired(),!dataRef.expired()})});
        }std::cout<<rows.toJSON()->dump();return 0;
    }
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto output=JSValue::array();
    for(unsigned i=0;i<requests.length();++i)call(output,"push",{run(requests.get(std::to_string(i)))});std::cout<<output.toJSON()->dump();return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

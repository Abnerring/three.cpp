#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/JSNumber.h"
#include "core/JSString.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue at(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)out.get("push").call(out,{value});return out;}
void log(JSValue state,const Args& values){const auto events=state.get("events");events.get("push").call(events,{array(values)});}
void tick(JSValue state,const std::string& label){log(state,{label});if(state.get("request").get("throwAt").strictEquals(label))throw JSTypeError(label);}
JSValue normal(const JSValue& value){
    if(value.isUndefined())return "#undefined";
    if(value.isSymbol())return "#symbol";
    if(value.type()==JSValue::Type::Number){const auto number=value.number();if(std::isnan(number))return "#NaN";if(std::isinf(number))return number>0?"#Infinity":"#-Infinity";if(number==0&&std::signbit(number))return "#-0";}
    return value;
}
JSValue decode(const JSValue& value){
    if(!value.isObject())return value;
    const auto tag=value.get("tag");
    if(tag.strictEquals("undefined"))return {};
    if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();
    if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();
    if(tag.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();
    if(tag.strictEquals("-0"))return -0.0;
    if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("object"))return JSValue::object();
    if(tag.strictEquals("boxedNumber"))return createNativeNumberObject(decode(value.get("value")));
    if(tag.strictEquals("boxedString"))return createNativeStringObject(decode(value.get("value")));
    return value;
}
JSValue error(std::exception_ptr exception){try{std::rethrow_exception(exception);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const JSReferenceError&){return "#ReferenceError";}catch(const std::exception& exception){return std::string("#Error:")+exception.what();}}
void data(JSValue object,const JSValue& key,JSValue value){JSPropertyDescriptor descriptor;descriptor.value=value;descriptor.writable=descriptor.configurable=true;object.defineProperty(key,descriptor);}
JSValue protocolValue(JSValue state,const JSValue& definition){
    const auto tag=definition.get("tag");
    if(!tag.strictEquals("primitive")&&!tag.strictEquals("fallback"))return decode(definition);
    auto object=JSValue::object();
    const auto install=[&](const JSValue& key,const std::string& method){
        JSPropertyDescriptor descriptor;descriptor.configurable=true;
        descriptor.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& captures)->JSValue{
            const auto state=captures[0],definition=captures[1],method=captures[2];
            const auto name=definition.get("name").string();tick(state,name+".get."+method.string());
            if(definition.get("noncallable").strictEquals(method))return 3;
            return JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{
                const auto state=captures[0],definition=captures[1],method=captures[2];
                const auto name=definition.get("name").string();tick(state,name+".call."+method.string());
                log(state,{name+".receiver",self.sameValue(captures[3])});
                if(method.strictEquals("toPrimitive"))log(state,{name+".hint",at(args,0)});
                if(definition.get("nested").toBoolean()){
                    const auto result=state.get("function").call(nullptr,{8,-3});log(state,{name+".nested",normal(result)});
                }
                if(definition.get("replaceMethod").toBoolean()){
                    auto math=state.get("math");data(math,state.get("request").get("name"),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 77;},{},"replacement",0));
                    log(state,{name+".replaceMethod"});
                }
                if(definition.get("replaceGlobal").toBoolean()){
                    auto replacement=JSValue::object();data(replacement,state.get("request").get("name"),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 88;},{},"replacement",0));
                    auto global=nativeGlobalObject();global.set("Math",replacement);log(state,{name+".replaceGlobal"});
                }
                if(definition.get("gc").toBoolean()){state.collectGarbage();log(state,{name+".gc"});}
                if(method.strictEquals("valueOf")&&definition.get("valueOfObject").toBoolean())return JSValue::object();
                return decode(definition.get("value"));
            },{state,definition,method,receiver},method.string(),method.strictEquals("toPrimitive")?1:0);
        },{state,definition,method},"get "+method,0);
        object.defineProperty(key,descriptor);
    };
    if(tag.strictEquals("primitive"))install(JSValue::wellKnownSymbol("toPrimitive"),"toPrimitive");
    else {install("valueOf","valueOf");install("toString","toString");}
    if(definition.get("proxy").toBoolean()){
        auto handler=JSValue::object();data(handler,"get",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
            const auto state=captures[0],name=captures[1];const auto key=at(args,1);
            tick(state,name.string()+".proxy.get."+(key.isSymbol()?std::string("toPrimitive"):key.toString()));
            return at(args,0).get(key,at(args,2));
        },{state,definition.get("name")},"get",3));
        object=JSValue::proxy(object,handler);
    }
    return object;
}
JSValue descriptorFlags(const std::optional<JSPropertyDescriptor>& descriptor){
    if(!descriptor)return {};
    return array({descriptor->writable.value_or(false),descriptor->enumerable.value_or(false),descriptor->configurable.value_or(false)});
}
JSValue metadata(){
    const auto math=nativeGlobalBinding("Math");auto out=JSValue::object();
    for(const auto* name:{"min","abs","max","ceil","round","sqrt","sin","cos","random"}){
        const auto function=math.get(name);auto record=JSValue::object();
        record.set("identity",function.sameValue(nativeGlobalBinding("Math").get(name)));
        record.set("name",function.get("name"));record.set("length",function.get("length"));record.set("constructible",function.isConstructible());record.set("ownPrototype",function.hasOwn("prototype"));
        record.set("functionPrototype",function.getPrototype().sameValue(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}).getPrototype()));
        record.set("nameFlags",descriptorFlags(function.getOwnPropertyDescriptor("name")));record.set("lengthFlags",descriptorFlags(function.getOwnPropertyDescriptor("length")));record.set("propertyFlags",descriptorFlags(math.getOwnPropertyDescriptor(name)));
        out.set(name,record);
    }
    out.set("globalMathFlags",descriptorFlags(nativeGlobalObject().getOwnPropertyDescriptor("Math")));
    out.set("tag",math.get(JSValue::wellKnownSymbol("toStringTag")));return out;
}
JSValue run(JSValue request){
    if(request.get("kind").strictEquals("metadata"))return metadata();
    auto state=JSValue::object();state.set("request",request);state.set("events",array());
    const auto math=nativeGlobalBinding("Math"),function=math.get(request.get("name"));state.set("math",math);state.set("function",function);
    Args args;const auto raw=request.get("args");for(unsigned i=0;i<raw.length();++i){const auto value=raw.get(std::to_string(i));args.push_back(value.isObject()?protocolValue(state,value):value);}
    auto out=JSValue::object();
    try{
        if(request.get("construct").toBoolean())out.set("result",normal(function.construct(args)));
        else out.set("result",normal(function.call(decode(request.get("receiver")),args)));
    }catch(...){out.set("result",error(std::current_exception()));}
    out.set("methodSame",math.get(request.get("name")).sameValue(function));out.set("globalSame",nativeGlobalBinding("Math").sameValue(math));out.set("events",state.get("events"));
    data(math,request.get("name"),function);auto global=nativeGlobalObject();global.set("Math",math);
    if(request.get("gcAfter").toBoolean())out.collectGarbage();
    return out;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(Animation_detail::parseJSON(line)).toJSON()->dump()<<'\n';}catch(const std::exception& exception){auto out=JSValue::object();out.set("probeError",exception.what());std::cout<<out.toJSON()->dump()<<'\n';}}}

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
// JSValue::trace preserves exact round-trip finite numbers and tags NaN, infinities and -0.
JSValue normal(const JSValue& value){
    if(value.type()==JSValue::Type::Number)return JSValue::fromJSON(value.trace());
    if(value.isUndefined())return "#undefined";
    if(value.isSymbol())return "#symbol";
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
    for(const auto* name:{"acos","asin","atan2"}){
        const auto function=math.get(name);auto record=JSValue::object();
        record.set("identity",function.sameValue(nativeGlobalBinding("Math").get(name)));
        record.set("source",function.nativeFunctionSource());record.set("name",function.get("name"));record.set("length",function.get("length"));record.set("constructible",function.isConstructible());record.set("ownPrototype",function.hasOwn("prototype"));
        record.set("functionPrototype",function.getPrototype().sameValue(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}).getPrototype()));
        record.set("nameFlags",descriptorFlags(function.getOwnPropertyDescriptor("name")));record.set("lengthFlags",descriptorFlags(function.getOwnPropertyDescriptor("length")));record.set("propertyFlags",descriptorFlags(math.getOwnPropertyDescriptor(name)));
        out.set(name,record);
    }
    out.set("PI",normal(math.get("PI")));out.set("PIFlags",descriptorFlags(math.getOwnPropertyDescriptor("PI")));out.set("globalMathFlags",descriptorFlags(nativeGlobalObject().getOwnPropertyDescriptor("Math")));
    out.set("tag",math.get(JSValue::wellKnownSymbol("toStringTag")));return out;
}
JSValue piCase(const JSValue& request){
    auto math=nativeGlobalBinding("Math"),out=JSValue::object(),state=JSValue::object();state.set("events",array());
    const auto operation=request.get("operation");JSValue result;
    try{
        if(operation.strictEquals("strictSet")){math.set("PI",17);result=17;}
        else if(operation.strictEquals("reflectSet"))result=math.reflectSet("PI",17,math);
        else if(operation.strictEquals("reflectDelete"))result=math.erase("PI");
        else {
            JSPropertyDescriptor d;
            if(operation.strictEquals("defineSame"))d.value=math.get("PI");
            else if(operation.strictEquals("defineValue"))d.value=17;
            else if(operation.strictEquals("defineWritable"))d.writable=true;
            else if(operation.strictEquals("defineEnumerable"))d.enumerable=true;
            else if(operation.strictEquals("defineConfigurable"))d.configurable=true;
            else if(operation.strictEquals("defineAccessor"))d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"get.PI"});return 17;},{state});
            else throw std::logic_error("Unknown PI fixture operation");
            math.defineProperty("PI",d);result=true;
        }
    }catch(...){result=error(std::current_exception());}
    out.set("result",normal(result));out.set("value",normal(math.get("PI")));out.set("flags",descriptorFlags(math.getOwnPropertyDescriptor("PI")));out.set("events",state.get("events"));return out;
}
JSValue bindingCase(const JSValue& request){
    auto global=nativeGlobalObject();const auto math=nativeGlobalBinding("Math"),fn=math.get(request.get("name"));auto out=JSValue::object(),state=JSValue::object();state.set("events",array());
    if(request.get("mode").strictEquals("deleted")){
        out.set("delete",global.erase("Math"));
        try{out.set("lookup",nativeGlobalBinding("Math"));}catch(...){out.set("lookup",error(std::current_exception()));}
    }else {
        JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"get.Math"});return c[1];},{state,math});global.defineProperty("Math",d);
        out.set("lookupSame",nativeGlobalBinding("Math").sameValue(math));
    }
    out.set("savedResult",normal(fn.call(nullptr,{.25,-.5})));state.collectGarbage();out.set("afterGC",normal(fn.call(nullptr,{.25,-.5})));
    data(global,"Math",math);out.set("mathSame",nativeGlobalBinding("Math").sameValue(math));out.set("functionSame",nativeGlobalBinding("Math").get(request.get("name")).sameValue(fn));out.set("events",state.get("events"));return out;
}
JSValue run(JSValue request){
    if(request.get("kind").strictEquals("metadata"))return metadata();
    if(request.get("kind").strictEquals("pi"))return piCase(request);
    if(request.get("kind").strictEquals("binding"))return bindingCase(request);
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
int main(int argc,char** argv){
 try{
  if(argc>1){if(argc!=2||std::string(argv[1])!="--lifetime")throw std::logic_error("Usage: probe [--lifetime]");
   for(unsigned realm=0;realm<3;++realm){JSWeakValue wm,wf;{const auto math=nativeGlobalBinding("Math"),fn=math.get("atan2");wm=JSWeakValue(math);wf=JSWeakValue(fn);const auto value=fn.call(nullptr,{-0.0,1});math.collectGarbage();if(!std::signbit(value.number())||!math.get("atan2").sameValue(fn))throw std::logic_error("Math identity/negative zero changed");}JSValue::nativeSafepoint();if(!wm.expired()||!wf.expired()||JSValue::hasActiveHeap())throw std::logic_error("Math realm retained");}
   std::cout<<"3 inverse-trig Math realm lifetimes passed\n";return 0;
  }
  const auto keeper=nativeGlobalObject();std::string line;while(std::getline(std::cin,line)){const auto input=Animation_detail::parseJSON(line);const auto execute=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return run(c[0]);},{input});std::cout<<execute.call().toJSON()->dump()<<'\n';}
 }catch(const std::exception& exception){std::cerr<<exception.what()<<'\n';return 1;}
}

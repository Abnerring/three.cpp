#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/JSJSON.h"
#include "core/JSBoolean.h"
#include "core/JSNumber.h"
#include "core/JSProxy.h"
#include "core/JSString.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace {
using Args=std::vector<JSValue>;
JSValue at(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue list(const Args& values={}){auto result=JSValue::array();for(std::size_t i=0;i<values.size();++i)result.set(std::to_string(i),values[i]);return result;}
void data(JSValue object,const JSValue& key,const JSValue& value,bool enumerable=true){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=true;d.enumerable=enumerable;object.defineProperty(key,d);}
void event(JSValue state,const Args& values){auto events=state.get("events");events.set(std::to_string(events.length()),list(values));}
JSValue flagList(const JSPropertyDescriptor& d){return list({d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)});}
std::string keyName(const JSValue& key){return key.isSymbol()?"@@"+key.symbolDescription().value_or(""):key.toString();}
JSValue simple(const JSValue& value){
    if(value.type()==JSValue::Type::Number)return JSValue::fromJSON(value.trace());
    if(value.isUndefined())return "#undefined";if(value.isSymbol())return "#symbol";if(value.isCallable())return "#function";
    if(value.isObject())return "#object";return value;
}
JSValue normal(const JSValue& value,std::vector<JSValue>& seen){
    if(!value.isObject()||value.isCallable())return simple(value);
    for(std::size_t i=0;i<seen.size();++i)if(seen[i].sameValue(value)){auto result=JSValue::object();result.set("reference",i);return result;}
    seen.push_back(value);const bool array=isNativeArray(value);auto result=JSValue::object(),entries=list();
    result.set("type",array?"array":"object");if(array)result.set("length",value.get("length"));
    for(const auto& key:value.ownPropertyNames()){
        if(array&&key=="length")continue;const auto descriptor=value.getOwnPropertyDescriptor(key);
        if(descriptor&&descriptor->enumerable.value_or(false))entries.set(std::to_string(entries.length()),list({key,normal(value.get(key),seen),flagList(*descriptor)}));
    }
    result.set("entries",entries);return result;
}
JSValue normal(const JSValue& value){std::vector<JSValue> seen;return normal(value,seen);}
JSValue build(const JSValue&,JSValue);
void effects(JSValue state,const JSValue& definition,JSValue self){
    if(definition.get("gc").toBoolean()){state.collectGarbage();event(state,{"gc"});}
    if(definition.get("nested").toBoolean())event(state,{"nested",nativeJSONStringify(nativeJSONParse("{\"nested\":[1,null]}"))});
    const auto mutations=definition.get("mutations");
    if(mutations.isObject())for(unsigned i=0;i<mutations.length();++i){
        const auto op=mutations.get(std::to_string(i)),kind=op.get("op"),key=op.get("key");
        if(kind.strictEquals("delete"))static_cast<void>(self.erase(key));
        else if(kind.strictEquals("readonly")){JSPropertyDescriptor d;d.value=build(op.get("value"),state);d.writable=d.configurable=false;d.enumerable=true;self.defineProperty(key,d);}
        else if(kind.strictEquals("preventExtensions"))self.preventExtensions();
        else self.set(key,build(op.get("value"),state));
        event(state,{"mutation",kind,key.isUndefined()?JSValue(""):key});
    }
    if(definition.get("throw").toBoolean())throw JSTypeError("fixture");
}
JSValue callback(JSValue state,JSValue definition,const std::string& role){
    return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        auto state=c[0];const auto definition=c[1],role=c[2],key=at(args,0),value=role.strictEquals("toJSON")?self:at(args,1);
        const auto context=at(args,2);JSValue source="#none";
        if(role.strictEquals("reviver")&&context.hasOwn("source"))source=context.get("source");
        event(state,{role,key,simple(value),args.size(),source});effects(state,definition,self);
        const auto rules=definition.get("rules");if(rules.isObject())for(unsigned i=0;i<rules.length();++i){const auto rule=rules.get(std::to_string(i));if(rule.get("key").sameValue(key)){effects(state,rule,self);if(rule.hasOwn("result"))return build(rule.get("result"),state);}}
        if(definition.get("mode").strictEquals("numbersPlus10")&&value.type()==JSValue::Type::Number)return value.number()+10;
        if(definition.get("mode").strictEquals("omitDrop")&&key.strictEquals("drop"))return {};
        if(definition.hasOwn("result"))return build(definition.get("result"),state);return value;
    },{state,definition,role});
}
JSValue build(const JSValue& definition,JSValue state){
    if(!definition.isObject())return definition;
    const auto type=definition.get("$");
    if(type.strictEquals("undefined")||type.strictEquals("hole"))return {};
    if(type.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();
    if(type.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();
    if(type.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();
    if(type.strictEquals("-0"))return -0.0;
    if(type.strictEquals("symbol"))return JSValue::symbol(definition.get("name").isUndefined()?"fixture":definition.get("name").string());
    if(type.strictEquals("ref"))return state.get("refs").get(definition.get("id"));
    if(type.strictEquals("raw"))return nativeJSONObject().get("rawJSON").call({}, {build(definition.get("value"),state)});
    if(type.strictEquals("function"))return callback(state,definition,"function");
    if(type.strictEquals("boxedNumber")||type.strictEquals("boxedString")||type.strictEquals("boxedBoolean")||type.strictEquals("convert")){
        JSValue object;
        if(type.strictEquals("convert"))object=JSValue::object();
        else{const auto value=build(definition.get("value"),state);object=type.strictEquals("boxedNumber")?createNativeNumberObject(value.toNumber()):type.strictEquals("boxedString")?createNativeStringObject(value.toString()):createNativeBooleanObject(value.toBoolean());}
        if(definition.get("id").toBoolean()){auto refs=state.get("refs");refs.set(definition.get("id"),object);}
        if(type.strictEquals("convert")||definition.hasOwn("convert")){
            data(object,JSValue::wellKnownSymbol("toPrimitive"),JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
                auto state=c[0];const auto definition=c[1],type=definition.get("$");auto name=definition.get("name");if(name.isUndefined())name=type.strictEquals("convert")?JSValue(""):type;
                event(state,{"convert",name,at(args,0)});effects(state,definition,self);
                return build(definition.get(type.strictEquals("convert")?"value":"convert"),state);
            },{state,definition}));
        }
        return object;
    }
    if(type.strictEquals("proxy")){
        const auto target=build(definition.get("value"),state);auto handler=JSValue::object();
        for(const auto* trap:{"get","ownKeys","getOwnPropertyDescriptor","defineProperty","deleteProperty"}){
            data(handler,trap,JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
                auto state=c[0],target=at(a,0);const auto definition=c[1],trap=c[2],key=at(a,1);const auto name=key.isUndefined()?std::string{}:keyName(key);
                if(trap.strictEquals("get")){event(state,{"get",name});if(definition.get("throwGet").strictEquals(name))throw JSTypeError("get");const auto overrides=definition.get("overrides");if(overrides.isObject())for(unsigned i=0;i<overrides.length();++i){const auto item=overrides.get(std::to_string(i));if(item.get("key").strictEquals(name))return build(item.get("value"),state);}return target.get(key,at(a,2));}
                if(trap.strictEquals("ownKeys")){event(state,{"ownKeys"});if(definition.get("throwKeys").toBoolean())throw JSTypeError("keys");if(definition.hasOwn("keys"))return definition.get("keys");return list(target.ownPropertyKeys());}
                if(trap.strictEquals("getOwnPropertyDescriptor")){event(state,{"descriptor",name});if(definition.get("throwDescriptor").strictEquals(name))throw JSTypeError("descriptor");const auto d=target.getOwnPropertyDescriptor(key);if(!d)return {};auto result=JSValue::object();if(d->value)result.set("value",*d->value);if(d->getter)result.set("get",*d->getter);if(d->setter)result.set("set",*d->setter);if(d->writable)result.set("writable",*d->writable);result.set("enumerable",d->enumerable.value_or(false));result.set("configurable",d->configurable.value_or(false));return result;}
                if(trap.strictEquals("deleteProperty")){event(state,{"delete",name});return target.erase(key);}
                event(state,{"define",name});const auto input=at(a,2);JSPropertyDescriptor d;
                if(input.hasOwn("value"))d.value=input.get("value");if(input.hasOwn("get"))d.getter=input.get("get");if(input.hasOwn("set"))d.setter=input.get("set");
                if(input.hasOwn("writable"))d.writable=input.get("writable").toBoolean();if(input.hasOwn("enumerable"))d.enumerable=input.get("enumerable").toBoolean();if(input.hasOwn("configurable"))d.configurable=input.get("configurable").toBoolean();return target.defineOwnProperty(key,d);
            },{state,definition,trap}));
        }
        auto result=JSValue::proxy(target,handler);if(definition.get("revoked").toBoolean())revokeNativeProxy(result);return result;
    }
    if(type.strictEquals("array")||type.strictEquals("object")){
        auto object=type.strictEquals("array")?JSValue::array():JSValue::object();
        if(definition.get("id").toBoolean()){auto refs=state.get("refs");refs.set(definition.get("id"),object);}
        if(type.strictEquals("array")){
            const auto items=definition.get("items");const auto count=items.isObject()?items.length():0;
            for(unsigned i=0;i<count;++i){const auto item=items.get(std::to_string(i));if(!item.isObject()||!item.get("$").strictEquals("hole"))data(object,std::to_string(i),build(item,state));}object.set("length",count);
        }
        const auto properties=definition.get("props");if(properties.isObject())for(unsigned i=0;i<properties.length();++i){
            const auto p=properties.get(std::to_string(i));const auto key=p.get("symbol").toBoolean()?JSValue::symbol(p.get("key").string()):p.get("key");JSPropertyDescriptor d;
            d.enumerable=p.get("enumerable").isUndefined()||p.get("enumerable").toBoolean();d.configurable=p.get("configurable").isUndefined()||p.get("configurable").toBoolean();
            if(p.get("getter").toBoolean())d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{auto state=c[0];event(state,{"getter",keyName(c[2])});effects(state,c[1],self);return build(c[1].get("value"),state);},{state,p,key});
            else{d.value=build(p.get("value"),state);d.writable=p.get("writable").isUndefined()||p.get("writable").toBoolean();}
            object.defineProperty(key,d);
        }
        if(definition.get("prototype").toBoolean())object.setPrototype(build(definition.get("prototype"),state));
        if(definition.get("toJSON").toBoolean())data(object,"toJSON",callback(state,definition.get("toJSON"),"toJSON"),false);
        return object;
    }
    throw std::logic_error("Unknown fixture type");
}
JSValue metadata(){
    const auto object=nativeGlobalBinding("JSON");auto out=JSValue::object(),keys=list();for(const auto& key:object.ownPropertyKeys())keys.set(std::to_string(keys.length()),keyName(key));out.set("keys",keys);
    out.set("tag",object.get(JSValue::wellKnownSymbol("toStringTag")));out.set("tagFlags",flagList(*object.getOwnPropertyDescriptor(JSValue::wellKnownSymbol("toStringTag"))));out.set("globalFlags",flagList(*nativeGlobalObject().getOwnPropertyDescriptor("JSON")));out.set("objectPrototype",object.getPrototype().sameValue(JSValue::object().getPrototype()));
    for(const auto* name:{"parse","stringify","rawJSON","isRawJSON"}){const auto fn=object.get(name);auto item=JSValue::object();item.set("name",fn.get("name"));item.set("length",fn.get("length"));item.set("source",fn.nativeFunctionSource());item.set("identity",fn.sameValue(object.get(name)));item.set("constructible",fn.isConstructible());item.set("prototype",fn.hasOwn("prototype"));item.set("nameFlags",flagList(*fn.getOwnPropertyDescriptor("name")));item.set("lengthFlags",flagList(*fn.getOwnPropertyDescriptor("length")));item.set("propertyFlags",flagList(*object.getOwnPropertyDescriptor(name)));out.set(name,item);}
    return out;
}
JSValue error(){try{throw;}catch(const JSJSONSyntaxError&){return "#SyntaxError";}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const JSReferenceError&){return "#ReferenceError";}catch(const std::exception& e){return std::string("#Error:")+e.what();}}
JSValue run(const JSValue& request){
    const auto kind=request.get("kind");if(kind.strictEquals("metadata"))return metadata();
    auto state=JSValue::object();state.set("events",list());state.set("refs",JSValue::object());auto out=JSValue::object();
    try{
        JSValue result;const auto json=nativeGlobalBinding("JSON");
        if(kind.strictEquals("parseDepth")){result=json.get("parse").call({}, {request.get("text")});unsigned depth=0;while(isNativeArray(result)){++depth;result=result.get("0");}out.set("depth",depth);}
        else if(kind.strictEquals("parse")){
            const auto function=json.get("parse");
            const auto receiver=build(request.get("receiver"),state),text=build(request.get("text"),state);
            const auto reviver=request.get("reviver").toBoolean()?callback(state,request.get("reviver"),"reviver"):build(request.get("reviverValue"),state);
            const auto extra=build(request.get("extra"),state);result=function.call(receiver,{text,reviver,extra});
        }else if(kind.strictEquals("stringify")){
            const auto function=json.get("stringify");
            const auto receiver=build(request.get("receiver"),state),value=build(request.get("value"),state);
            const auto replacer=request.get("replacer").toBoolean()?callback(state,request.get("replacer"),"replacer"):build(request.get("replacerValue"),state);
            const auto space=build(request.get("space"),state),extra=build(request.get("extra"),state);result=function.call(receiver,{value,replacer,space,extra});
        }else if(kind.strictEquals("raw")){
            result=json.get("rawJSON").call({}, {build(request.get("text"),state)});out.set("brand",json.get("isRawJSON").call({}, {result}));out.set("proxyBrand",json.get("isRawJSON").call({}, {JSValue::proxy(result,JSValue::object())}));out.set("nullPrototype",result.getPrototype().isNull());out.set("extensible",result.isExtensible());out.set("flags",flagList(*result.getOwnPropertyDescriptor("rawJSON")));out.set("stringified",json.get("stringify").call({}, {result}));
        }else if(kind.strictEquals("isRaw"))result=json.get("isRawJSON").call({}, {build(request.get("value"),state)});
        else if(kind.strictEquals("construct"))result=json.get(request.get("name")).construct({build(request.get("value"),state)});
        else if(kind.strictEquals("binding")){
            auto global=nativeGlobalObject();const auto fn=json.get("stringify");global.erase("JSON");auto input=JSValue::object();input.set("x",1);out.set("saved",fn.call({}, {input}));try{static_cast<void>(nativeGlobalBinding("JSON"));}catch(...){out.set("lookup",error());}data(global,"JSON",json,false);state.collectGarbage();result=json.get("stringify").sameValue(fn);
        }else throw std::logic_error("Unknown fixture kind");
        out.set("result",normal(result));
    }catch(...){out.set("result",error());}
    out.set("events",state.get("events"));return out;
}
}
int main(int argc,char** argv){
    try{
        if(argc>1){if(argc!=2||std::string(argv[1])!="--lifetime")throw std::logic_error("Usage: probe [--lifetime]");
            for(unsigned realm=0;realm<3;++realm){JSWeakValue module,method;{const auto json=nativeGlobalBinding("JSON"),fn=json.get("parse");module=JSWeakValue(json);method=JSWeakValue(fn);const auto value=fn.call({}, {"{\"a\":1}"});json.collectGarbage();if(value.get("a").number()!=1||!nativeJSONObject().sameValue(json))throw std::logic_error("JSON identity changed");}JSValue::nativeSafepoint();if(!module.expired()||!method.expired()||JSValue::hasActiveHeap())throw std::logic_error("JSON realm retained");}std::cout<<"3 JSON realm lifetimes passed\n";return 0;
        }
        const auto keeper=nativeGlobalObject();std::string line;
        while(std::getline(std::cin,line)){const auto request=Animation_detail::parseJSON(line);const auto execute=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return run(c[0]);},{request});std::cout<<execute.call().toJSON()->dump()<<'\n';}
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

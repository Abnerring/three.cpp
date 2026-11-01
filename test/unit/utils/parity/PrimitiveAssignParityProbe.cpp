#include "core/JSValue.h"
#include "core/JSObject.h"
#include "core/JSSymbol.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void emit(const JsonValue& value){std::cout<<value.dump()<<'\n';}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JsonValue encode(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;if(value.isSymbol())return "symbol:"+value.symbolDescription().value_or("");if(value.isObject())return "object";if(value.type()==JSValue::Type::Number)return value.number();if(value.type()==JSValue::Type::Boolean)return value.toBoolean();return value.string();}
void append(JSValue log,const JSValue& value){log.set(std::to_string(log.length()),value);}
JSValue primitive(unsigned i){switch(i){case 0:return 3;case 1:return "abc";case 2:return false;default:return JSSymbol::create("receiver");}}
JSValue dynamicKey(const JSValue& log,const JSValue& symbol,unsigned mode){
    auto key=JSValue::object();JSPropertyDescriptor hook;
    hook.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        append(c[0],"get");if(c[2].number()==2)throw JSRangeError("get");if(c[2].number()==4)return 7;
        return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
            append(c[0],args[0]);if(c[2].number()==3)throw JSRangeError("call");if(c[2].number()==5)return JSValue::object();
            return c[2].number()==0?JSValue("__collection_key__"):c[1];
        },c);
    },{log,symbol,mode});hook.configurable=true;key.defineProperty(JSSymbol::toPrimitive(),hook);return key;
}
}
int main(){
    const auto object=JSValue::intrinsicObjectConstructor(),assign=object.get("assign");
    for(unsigned receiver=0;receiver<2;++receiver){
        const auto target=receiver==0?JSValue(3):JSSymbol::create("boxed-target");
        auto input=JSValue::object();const auto symbol=JSSymbol::create("assigned");input.set("a",11);input.set("b",12);input.set("c",13);input.set(symbol,14);
        const auto box=assign.call(object,{target,input}),other=assign.call(object,{target,input});
        A keys,descriptors;for(const auto& key:box.ownPropertyKeys()){
            keys.push_back(encode(key));const auto d=box.getOwnPropertyDescriptor(key);
            if(!d||!d->value||!d->writable||!d->enumerable||!d->configurable)throw std::runtime_error("Incomplete assigned descriptor");
            descriptors.push_back(O{{"key",encode(key)},{"value",encode(*d->value)},{"writable",*d->writable},{"enumerable",*d->enumerable},{"configurable",*d->configurable}});
        }
        emit(O{{"kind","assign-box"},{"receiver",receiver},{"object",box.isObject()},{"distinctFromPrimitive",!box.sameValue(target)},
            {"valueOfSame",call(box,"valueOf").sameValue(target)},{"prototypeSame",box.getPrototype().sameValue(target.toObject().getPrototype())},
            {"freshBox",!box.sameValue(other)},{"existingIdentity",assign.call(object,{box,JSValue::object(),nullptr,JSValue{}}).sameValue(box)},
            {"keys",keys},{"descriptors",descriptors}});
    }
    for(unsigned receiver=0;receiver<2;++receiver){
        auto trace=JSValue::array(),input=JSValue::object();JSPropertyDescriptor d;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],"read-source");return 9;},{trace});d.enumerable=true;input.defineProperty("a",d);
        std::string outcome="ok";try{assign.call(object,{receiver==0?JSValue{}:JSValue(nullptr),input});}catch(const JSTypeError&){outcome="TypeError";}
        emit(O{{"kind","nullish-assign"},{"receiver",receiver},{"outcome",outcome},{"log",*trace.toJSON()}});
    }
    // Both overloads use the original primitive as the inherited setter receiver.
    for(unsigned receiver=0;receiver<4;++receiver)for(unsigned keyKind=0;keyKind<3;++keyKind){
        auto target=primitive(receiver),prototype=target.toObject().getPrototype(),log=JSValue::array();
        const auto key=keyKind==2?JSSymbol::create("setter"):JSValue("__threecpp_primitive_assign_probe__");
        if(prototype.hasOwn(key))throw std::runtime_error("Probe property already present");
        JSPropertyDescriptor d;d.configurable=true;
        d.setter=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            append(c[0],self.sameValue(c[1]));append(c[0],args[0]);c[0].collectGarbage();return {};
        },{log,target});prototype.defineProperty(key,d);
        std::string outcome="ok";try{if(keyKind==0)target.set(std::string("__threecpp_primitive_assign_probe__"),9);else target.set(key,9);}catch(const JSTypeError&){outcome="TypeError";}
        const bool noOwn=!target.toObject().hasOwn(key);prototype.erase(key);
        emit(O{{"kind","inherited-setter"},{"receiver",receiver},{"keyKind",keyKind},{"outcome",outcome},{"log",*log.toJSON()},{"noOwn",noOwn}});
    }
    // A String wrapper's own read-only index shadows an inherited setter.
    for(unsigned keyKind=0;keyKind<2;++keyKind){
        JSValue target="abc";auto prototype=target.toObject().getPrototype(),log=JSValue::array();
        if(prototype.hasOwn("0"))throw std::runtime_error("Probe index already present");
        JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],"unexpected-setter");return {};},{log});prototype.defineProperty("0",d);
        std::string outcome="ok";try{if(keyKind==0)target.set(std::string("0"),9);else target.set(JSValue("0"),9);}catch(const JSTypeError&){outcome="TypeError";}
        const auto index=target.toObject().get("0");prototype.erase("0");
        emit(O{{"kind","readonly-string-index"},{"keyKind",keyKind},{"outcome",outcome},{"log",*log.toJSON()},{"index",encode(index)}});
    }
    for(unsigned receiver=0;receiver<2;++receiver)for(unsigned operation=0;operation<2;++operation)for(unsigned mode=0;mode<6;++mode){
        auto target=primitive(receiver),log=JSValue::array();const auto symbol=JSSymbol::create("key"),key=dynamicKey(log,symbol,mode);
        JsonValue outcome="ok";try{if(operation==0)target.set(key,9);else outcome=target.reflectSet(key,9,target);}catch(const JSTypeError&){outcome="TypeError";}catch(const JSRangeError&){outcome="RangeError";}
        emit(O{{"kind","strict-vs-reflect"},{"receiver",receiver},{"operation",operation},{"mode",mode},{"outcome",outcome},{"log",*log.toJSON()}});
    }
}

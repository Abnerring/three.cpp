#include "core/JSValue.h"
#include "core/JSSymbol.h"
#include <cmath>
#include <iostream>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JsonValue encode(const JSValue& v){if(v.isUndefined())return "undefined";if(v.isNull())return nullptr;if(v.isSymbol())return "symbol:"+v.symbolDescription().value_or("");if(v.isCallable())return "function";if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "NaN";if(v.number()==0&&std::signbit(v.number()))return "-0";return v.number();}if(v.type()==JSValue::Type::String)return v.string();if(v.type()==JSValue::Type::Boolean)return v.toBoolean();return "object";}
JsonValue descriptor(const std::optional<JSPropertyDescriptor>& d){if(!d)return nullptr;O o;if(d->value){o["value"]=encode(*d->value);o["writable"]=*d->writable;}else{o["get"]=encode(*d->getter);o["set"]=encode(*d->setter);}o["enumerable"]=*d->enumerable;o["configurable"]=*d->configurable;return o;}
void emit(const JsonValue& v){std::cout<<v.dump()<<'\n';}
JSValue getter(const JSValue&,const Args&,const Args&){return 7;}
JSValue setter(const JSValue& self,const Args& args,const Args&){auto target=self;target.set("setValue",args[0]);return {};}
}
int main(){
    const auto get=JSValue::function(getter),set=JSValue::function(setter);
    for(unsigned original=0;original<5;++original)for(unsigned flags=0;flags<8;++flags)for(unsigned update=0;update<13;++update){
        auto object=JSValue::object();const auto key=JSSymbol::create("key");
        if(original){JSPropertyDescriptor d;d.configurable=bool(flags&1);d.enumerable=bool(flags&2);if(original<3){d.value=original==1?JSValue(0):JSValue(std::nan(""));d.writable=bool(flags&4);}else if(original==3)d.getter=get;else d.setter=set;object.defineProperty(key,d);}
        JSPropertyDescriptor change;
        switch(update){case 0:break;case 1:change.value=0;break;case 2:change.value=-0.0;break;case 3:change.value=std::nan("");break;case 4:change.writable=false;break;case 5:change.writable=true;break;case 6:change.enumerable=!bool(flags&2);break;case 7:change.configurable=true;break;case 8:change.getter=get;break;case 9:change.getter=JSValue::function(getter);break;case 10:change.setter=set;break;case 11:change.getter=JSValue{};break;case 12:change.getter=get;change.value=9;break;}
        JsonValue outcome;try{outcome=object.defineOwnProperty(key,change);}catch(const JSTypeError&){outcome="TypeError";}
        emit(O{{"kind","descriptor"},{"original",original},{"flags",flags},{"update",update},{"outcome",outcome},{"descriptor",descriptor(object.getOwnPropertyDescriptor(key))}});
    }
    for(unsigned inherited=0;inherited<5;++inherited)for(unsigned receiver=0;receiver<5;++receiver){
        const auto key=JSSymbol::create("key");auto prototype=JSValue::object(),target=JSValue::object(prototype);
        if(inherited){JSPropertyDescriptor d;d.configurable=true;if(inherited<3){d.value=1;d.writable=inherited==1;}else if(inherited==3)d.getter=get;else d.setter=set;prototype.defineProperty(key,d);}
        if(receiver==1)target.preventExtensions();if(receiver>=2&&receiver<4){JSPropertyDescriptor d;if(receiver==2){d.value=2;d.writable=false;}else d.getter=get;d.configurable=true;target.defineProperty(key,d);}
        JsonValue result;try{result=prototype.reflectSet(key,9,receiver==4?JSValue(12):target);}catch(const JSTypeError&){result="TypeError";}
        emit(O{{"kind","inherited"},{"inherited",inherited},{"receiver",receiver},{"outcome",result},{"value",encode(target.get(key))},{"setValue",encode(target.get("setValue"))},{"own",target.hasOwn(key)}});
    }
    for(unsigned mode=0;mode<8;++mode){
        auto object=JSValue::object(),keyObject=JSValue::object(),log=JSValue::array();const auto symbol=JSSymbol::create("key");
        JSPropertyDescriptor hook;hook.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            call(c[0],"push",{"get"});if(c[2].number()==1)throw JSRangeError("get");if(c[2].number()==2)return 3;if(c[2].number()==3)return nullptr;
            return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{call(c[0],"push",{args[0]});const auto m=c[2].number();if(m==4)throw JSRangeError("call");if(m==5)return JSValue::object();if(m==6)return "string-key";return c[1];},c);
        },{log,symbol,mode});hook.configurable=true;keyObject.defineProperty(JSSymbol::toPrimitive(),hook);
        JsonValue outcome;try{object.set(keyObject,9);outcome="ok";}catch(const JSTypeError&){outcome="TypeError";}catch(const JSRangeError&){outcome="RangeError";}
        A names;for(const auto& key:object.ownPropertyKeys())names.push_back(encode(key));
        emit(O{{"kind","coercion"},{"mode",mode},{"outcome",outcome},{"keys",names},{"log",*log.toJSON()}});
    }
    auto object=JSValue::object();const auto a=JSSymbol::create("same"),b=JSSymbol::create("same");object.set(a,1);object.set("b",2);object.set("1",3);object.set(b,4);object.set("a",5);
    object.erase(a);object.set(a,6);A names;for(const auto& key:object.ownPropertyKeys())names.push_back(encode(key));
    emit(O{{"kind","order"},{"keys",names},{"identity",!a.sameValue(b)},{"first",object.ownPropertySymbols()[0].sameValue(b)},{"second",object.ownPropertySymbols()[1].sameValue(a)},{"json",object.toJSON()->dump()}});
    // Property operators and Reflect validate their target before ToPropertyKey.
    // The two legacy Object.prototype methods intentionally convert key first.
    for(unsigned operation=0;operation<10;++operation)for(unsigned receiver=0;receiver<9;++receiver)for(unsigned mode=0;mode<6;++mode){
        // Primitive boxing in propertyIsEnumerable predates this key protocol.
        if(operation==9&&receiver>=2&&receiver<=5)continue;
        JSValue target;
        switch(receiver){case 0:break;case 1:target=nullptr;break;case 2:target=false;break;case 3:target=0;break;case 4:target="";break;case 5:target=JSSymbol::create("receiver");break;case 6:target=JSValue::object();break;case 7:target=JSValue::array();break;case 8:target=JSValue::function(getter);break;}
        auto log=JSValue::array(),dynamicKey=JSValue::object();const auto symbol=JSSymbol::create("key");
        JSPropertyDescriptor hook;hook.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            call(c[0],"push",{"get"});const auto mode=c[2].number();if(mode==2)throw JSRangeError("get");if(mode==4)return 7;
            return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
                call(c[0],"push",{args[0]});const auto mode=c[2].number();if(mode==3)throw JSRangeError("call");if(mode==5)return JSValue::object();return mode==0?JSValue("__collection_key__"):c[1];
            },c);
        },{log,symbol,mode});dynamicKey.defineProperty(JSSymbol::toPrimitive(),hook);
        JsonValue outcome;
        try{
            switch(operation){
            case 0:outcome=encode(target.get(dynamicKey));break;
            case 1:outcome=target.has(dynamicKey);break;
            case 2:outcome=target.hasOwn(dynamicKey);break;
            case 3:outcome=descriptor(target.getOwnPropertyDescriptor(dynamicKey));break;
            case 4:outcome=target.reflectSet(dynamicKey,9,target);break;
            case 5:{JSPropertyDescriptor d;d.value=9;outcome=target.defineOwnProperty(dynamicKey,d);break;}
            case 6:outcome=target.erase(dynamicKey);break;
            case 7:target.set(dynamicKey,9);outcome="ok";break;
            case 8:outcome=encode(JSValue::object().get("hasOwnProperty").call(target,{dynamicKey}));break;
            case 9:outcome=encode(JSValue::object().get("propertyIsEnumerable").call(target,{dynamicKey}));break;
            }
        }catch(const JSTypeError&){outcome="TypeError";}catch(const JSRangeError&){outcome="RangeError";}
        emit(O{{"kind","receiver-key-order"},{"operation",operation},{"receiver",receiver},{"mode",mode},{"outcome",outcome},{"log",*log.toJSON()}});
    }
}

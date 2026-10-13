#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSIterator.h"
#include "core/JSSymbol.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
JSValue noop(const JSValue&,const Args&,const Args&) {return {};}
JSValue call(const JSValue& self,const char* name,const Args& args={}) {return self.get(name).call(self,args);}
JSValue tuple(const Args& values) {auto array=JSValue::array();for(const auto& value:values)array.set(std::to_string(array.length()),value);return array;}
void log(const JSValue& context,const char* name) {call(context.get("log"),"push",{name});}
void getter(JSValue object,const JSValue& key,JSValue::NativeFunction fn,const Args& captures) {JSPropertyDescriptor d;d.getter=JSValue::function(fn,captures);d.configurable=true;object.defineProperty(key,d);}
JsonValue encode(const JSValue& value) {
    if(value.isUndefined())return "@undefined";if(value.isNull())return nullptr;
    if(value.isSymbol())return "@symbol:"+value.symbolDescription().value_or("");
    if(value.type()==JSValue::Type::Number) {const auto n=value.number();if(std::isnan(n))return "@NaN";if(std::isinf(n))return n<0?"@-Infinity":"@Infinity";if(n==0&&std::signbit(n))return "@-0";return n;}
    if(value.type()==JSValue::Type::Boolean)return value.toBoolean();if(value.type()==JSValue::Type::String)return value.string();
    if(value.type()==JSValue::Type::Array){A array;for(unsigned i=0;i<value.length();++i)array.push_back(encode(value.get(std::to_string(i))));return array;}
    if(value.isCallable())return "@function";return "@object";
}
template<class F>JsonValue attempt(F&& callback) {try{return encode(callback());}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}}
JsonValue step(const JSValue& iterator) {const auto result=call(iterator,"next");return O{{"value",encode(result.get("value"))},{"done",result.get("done").toBoolean()}};}
JsonValue contents(const JSValue& object){auto iterator=getJSIterator(object);A array;while(const auto value=stepJSIterator(iterator))array.push_back(encode(*value));return array;}
void emit(const JsonValue& value){std::cout<<value.dump()<<'\n';}
Args keys(){return {{},nullptr,false,true,0,-0.0,1,1.5,std::nan(""),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),"","1",JSValue::object(),JSValue::array(),JSValue::function(noop),JSSymbol::create("x"),JSSymbol::create("x")};}
void constructors(bool map,unsigned failure,unsigned closeMode) {
    auto context=JSValue::object();context.set("log",JSValue::array());context.set("failure",failure);context.set("close",closeMode);
    auto iterator=JSValue::object(),iterable=JSValue::object(),entry=JSValue::object();iterator.set("index",0);
    getter(entry,"0",+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"key");if(c[0].get("failure").number()==8)throw JSRangeError("key");return "k";},{context});
    getter(entry,"1",+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"value");if(c[0].get("failure").number()==9)throw JSRangeError("value");return 6;},{context});
    getter(iterator,"next",+[](const JSValue&,const Args&,const Args& c)->JSValue{
        log(c[0],"getNext");if(c[0].get("failure").number()==3)throw JSRangeError("getNext");
        return JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {
            log(c[0],"next");const auto failure=c[0].get("failure").number();if(failure==4)throw JSRangeError("next");if(failure==5)return 1;
            auto receiver=self;const auto index=receiver.get("index").number();receiver.set("index",index+1);
            // Captured next must survive replacement after the first step.
            receiver.set("replacement",true);JSPropertyDescriptor next;next.value=17;next.writable=true;next.configurable=true;receiver.defineProperty("next",next);
            auto result=JSValue::object();
            getter(result,"done",+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"done");if(c[0].get("failure").number()==6)throw JSRangeError("done");return c[2].number()>0;},{c[0],c[1],index});
            getter(result,"value",+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"item");const auto f=c[0].get("failure").number();if(f==7)throw JSRangeError("item");return f==10?JSValue(12):c[1];},{c[0],c[1]});return result;
        },c);
    },{context,entry});
    getter(iterator,"return",+[](const JSValue&,const Args&,const Args& c)->JSValue{
        log(c[0],"getReturn");const auto mode=c[0].get("close").number();if(mode==1)throw JSTypeError("getReturn");if(mode==2)return 3;if(mode==3)return nullptr;if(mode==4)return {};
        return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"return");const auto mode=c[0].get("close").number();if(mode==5)throw JSTypeError("return");return mode==6?JSValue(3):JSValue::object();},c);
    },{context});
    getter(iterable,JSSymbol::iterator(),+[](const JSValue&,const Args&,const Args& c)->JSValue {
        log(c[0],"getIterator");const auto failure=c[0].get("failure").number();if(failure==1)throw JSRangeError("getIterator");if(failure==2)return 3;
        return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"iterator");return c[1];},c);
    },{context,iterator});
    const auto ctor=map?nativeMapConstructor():nativeSetConstructor();const auto adder=ctor.get("prototype").get(map?"set":"add");
    auto target=JSValue::constructor(noop,{},"Target",0,JSValue::ConstructorKind::Callable);auto prototype=JSValue::object(ctor.get("prototype"));target.set("prototype",prototype);
    getter(prototype,map?"set":"add",+[](const JSValue&,const Args&,const Args& c)->JSValue {
        log(c[0],"getAdder");const auto failure=c[0].get("failure").number();if(failure==11)throw JSRangeError("getAdder");if(failure==12)return 1;
        return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{log(c[0],"add");if(c[0].get("failure").number()==13)throw JSRangeError("adder");return c[1].call(self,args);},c);
    },{context,adder});
    JsonValue outcome;try{outcome=contents(ctor.construct({iterable},target));}catch(const JSTypeError&){outcome=O{{"error","TypeError"}};}catch(const JSRangeError&){outcome=O{{"error","RangeError"}};}
    emit(O{{"kind","constructor"},{"map",map},{"failure",failure},{"close",closeMode},{"outcome",outcome},{"log",encode(context.get("log"))}});
}
}
int main() {
    for(bool map:{true,false}) {
        const auto all=keys();const auto object=map?createNativeMap():createNativeSet();const auto add=map?"set":"add";
        for(unsigned i=0;i<all.size();++i) {
            const auto returned=call(object,add,{all[i],i});A has,values;
            for(const auto& key:all){has.push_back(encode(call(object,"has",{key})));if(map)values.push_back(encode(call(object,"get",{key})));}
            emit(O{{"kind","keys"},{"map",map},{"index",i},{"self",returned.sameValue(object)},{"size",encode(object.get("size"))},{"has",has},{"values",values},{"contents",contents(object)}});
        }
        for(unsigned i=0;i<all.size();++i)emit(O{{"kind","delete"},{"map",map},{"index",i},{"result",encode(call(object,"delete",{all[i]}))},{"size",encode(object.get("size"))},{"contents",contents(object)}});
        for(unsigned seed=1;seed<=16;++seed) {
            auto object=map?createNativeMap():createNativeSet();for(unsigned i=0;i<160;++i)call(object,add,{i,i+1000});
            Args iterators{call(object,"keys"),call(object,"values"),call(object,"entries")};std::uint32_t random=seed;
            for(unsigned operation=0;operation<350;++operation) {
                random^=random<<13;random^=random>>17;random^=random<<5;const auto key=random%180,mode=random%11;
                JsonValue result;
                if(mode<4)result=encode(call(object,"delete",{key}));
                else if(mode<7)result=call(object,add,{key,operation}).sameValue(object);
                else if(mode<10)result=step(iterators[mode-7]);
                else if(operation%13==0){call(object,"clear");result="clear";}
                else {iterators[key%3]=call(object,key%3==0?"keys":key%3==1?"values":"entries");result="iterator";}
                emit(O{{"kind","mutation"},{"map",map},{"seed",seed},{"operation",operation},{"result",result},{"size",encode(object.get("size"))}});
            }
            emit(O{{"kind","mutationFinal"},{"map",map},{"seed",seed},{"contents",contents(object)}});
        }
        for(unsigned mode=0;mode<9;++mode) {
            auto object=map?createNativeMap():createNativeSet(),state=JSValue::object(),context=JSValue::object();
            state.set("mutated",false);state.set("log",JSValue::array());
            for(unsigned i=0;i<3;++i)call(object,add,{i,i*10});
            const auto callback=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
                auto state=c[0];call(state.get("log"),"push",{tuple({args[0],args[1],args[2].sameValue(c[1]),self.sameValue(c[2])})});
                const auto mode=c[3].number();const auto add=c[4].toBoolean()?"set":"add";
                if(mode==7&&args[1].number()==1)throw JSRangeError("callback");
                if(!state.get("mutated").toBoolean()) {
                    state.set("mutated",true);
                    if(mode==1)call(c[1],"delete",{1});
                    if(mode==2){call(c[1],"delete",{0});call(c[1],add,{0,70});}
                    if(mode==3){call(c[1],"clear");call(c[1],add,{7,70});}
                    if(mode==4)call(c[1],add,{1,999});
                    if(mode==5)call(c[1],add,{7,70});
                    if(mode==6)call(c[1],add,{0,900});
                    if(mode==8)call(c[1],"forEach",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{call(c[0].get("log"),"push",{tuple({"nested",a[0],a[1]})});return {};},{state})});
                }
                return {};
            },{state,object,context,mode,map});
            const auto outcome=attempt([&]{return call(object,"forEach",{callback,context});});
            emit(O{{"kind","forEach"},{"map",map},{"mode",mode},{"outcome",outcome},{"log",encode(state.get("log"))},{"contents",contents(object)}});
        }
        for(unsigned failure=0;failure<14;++failure)for(unsigned close=0;close<7;++close)constructors(map,failure,close);
        const auto ctor=map?nativeMapConstructor():nativeSetConstructor(),proto=ctor.get("prototype");
        for(const auto* name:{"has","delete","clear","entries","forEach","keys","values"}) {
            const auto method=proto.get(name);const auto d=proto.getOwnPropertyDescriptor(name);
            emit(O{{"kind","metadata"},{"map",map},{"method",name},{"name",encode(method.get("name"))},{"length",encode(method.get("length"))},{"writable",*d->writable},{"enumerable",*d->enumerable},{"configurable",*d->configurable},{"constructible",method.isConstructible()}});
            for(unsigned i=0;i<all.size();++i)emit(O{{"kind","brand"},{"map",map},{"method",name},{"index",i},{"result",attempt([&]{return method.call(all[i],{JSValue::function(noop)});})}});
        }
        emit(O{{"kind","prototype"},{"map",map},{"tag",encode(proto.get(JSSymbol::toStringTag()))},{"iterAlias",proto.get(JSSymbol::iterator()).sameValue(proto.get(map?"entries":"values"))},{"species",ctor.get(JSSymbol::species()).sameValue(ctor)},{"call",attempt([&]{return ctor.call();})},{"sizeBrand",attempt([&]{return proto.get("size");})},{"keysValues",proto.get("keys").sameValue(proto.get("values"))}});
    }
    for(unsigned mode=0;mode<3;++mode) {
        auto object=JSValue::object(),state=JSValue::object();state.set("busy",false);state.set("events",JSValue::array());object.set("0","a");object.set("1","b");
        getter(object,"length",+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"length"});auto state=c[0];if(!state.get("busy").toBoolean()){state.set("busy",true);call(state.get("events"),"push",{call(state.get("iterator"),"next") .get("value")});}return 2;},{state});
        const auto iterator=JSIterator::array(object,static_cast<JSIterator::Kind>(mode));state.set("iterator",iterator);
        A results;results.push_back(step(iterator));results.push_back(step(iterator));results.push_back(step(iterator));
        emit(O{{"kind","arrayReentry"},{"mode",mode},{"results",results},{"events",encode(state.get("events"))}});
    }
    for(const auto& string:Args{"","ascii","A\xF0\x9F\x98\x80" "B","\xED\xA0\x80" "a","\xED\xB0\x80"}) {
        auto iterator=getJSIterator(string);A units;while(const auto value=stepJSIterator(iterator))units.push_back(encode(*value));
        emit(O{{"kind","string"},{"input",encode(string)},{"units",units}});
    }
}

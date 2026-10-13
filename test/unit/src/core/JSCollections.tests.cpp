#include "catch.hpp"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSIterator.h"
#include "core/JSSymbol.h"
#include <cmath>
#include <limits>
#include <thread>

namespace {
using Args=std::vector<JSValue>;
JSValue call(const JSValue& self,const char* name,const Args& args={}) {return self.get(name).call(self,args);}
JSValue tuple(const Args& args) {auto value=JSValue::array();for(const auto& item:args)value.set(std::to_string(value.length()),item);return value;}
JSValue next(const JSValue& iterator) {return call(iterator,"next");}
JSPropertyDescriptor data(const JSValue& value,bool configurable=true) {JSPropertyDescriptor d;d.value=value;d.writable=true;d.enumerable=true;d.configurable=configurable;return d;}
void getter(JSValue object,const JSValue& key,JSValue::NativeFunction fn,const Args& captures={}) {
    JSPropertyDescriptor d;d.getter=JSValue::function(fn,captures);d.configurable=true;object.defineProperty(key,d);
}
JSValue noop(const JSValue&,const Args&,const Args&) {return {};}
}
TEST_CASE("Symbols are distinct primitive property keys with ordinary descriptors", "[runtime][symbol]") {
    const auto a=JSSymbol::create("same"),b=JSSymbol::create("same");
    REQUIRE(a.isSymbol());REQUIRE_FALSE(a.isObject());REQUIRE_FALSE(a.sameValue(b));
    REQUIRE(a.toBoolean());REQUIRE_THROWS_AS(a.toString(),JSTypeError);REQUIRE_THROWS_AS(a.toNumber(),JSTypeError);
    REQUIRE(JSSymbol::iterator().sameValue(JSSymbol::iterator()));
    REQUIRE(a.symbolDescription()=="same");REQUIRE_FALSE(JSSymbol::create().symbolDescription());
    auto object=JSValue::object();object.set(a,1);object.set(b,2);object.set("2",3);object.set("z",4);
    REQUIRE(object.get(a).number()==1);REQUIRE(object.get(b).number()==2);
    REQUIRE(object.keys()==std::vector<std::string>{"2","z"});
    const auto keys=object.ownPropertyKeys();REQUIRE(keys.size()==4);REQUIRE(keys[2].sameValue(a));REQUIRE(keys[3].sameValue(b));
    object.erase(a);object.set(a,5);REQUIRE(object.ownPropertySymbols()[1].sameValue(a));
    auto child=JSValue::object(object);REQUIRE(child.get(b).number()==2);REQUIRE_FALSE(child.hasOwn(b));REQUIRE(child.has(b));
    child.set(b,9);REQUIRE(object.get(b).number()==2);REQUIRE(child.get(b).number()==9);
    object.defineProperty(a,data(7,false));REQUIRE_FALSE(object.erase(a));
    JSPropertyDescriptor accessor;accessor.getter=JSValue::function(noop);REQUIRE_FALSE(object.defineOwnProperty(a,accessor));
    object.set("symbolValue",b);auto array=tuple({b});
    REQUIRE(object.toJSON()->dump()=="{\"2\":3,\"z\":4}");REQUIRE(array.toJSON()->dump()=="[null]");REQUIRE_FALSE(a.toJSON());
}
TEST_CASE("Symbol property access observes receiver, conversion and inherited hooks", "[runtime][symbol]") {
    auto symbol=JSSymbol::create("slot"),key=JSValue::object(),prototype=JSValue::object(),child=JSValue::object(prototype);
    auto log=JSValue::array();
    getter(key,JSSymbol::toPrimitive(),+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto log=c[0];call(log,"push",{"get"});
        return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{call(c[0],"push",{a[0]});return c[1];},c);
    },{log,symbol});
    child.set(key,42);REQUIRE(log.toJSON()->dump()=="[\"get\",\"string\"]");REQUIRE(child.get(symbol).number()==42);
    child.erase(symbol);getter(prototype,symbol,+[](const JSValue& self,const Args&,const Args&)->JSValue{return self.get("marker");});
    child.set("marker",17);REQUIRE(child.get(symbol).number()==17);
    prototype.set(JSSymbol::toStringTag(),"Branded");REQUIRE(call(child,"toString").string()=="[object Branded]");
    auto input=JSValue::object(prototype);prototype.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 12;}));
    REQUIRE(input.toNumber()==12);prototype.set(JSSymbol::toPrimitive(),nullptr);REQUIRE(input.toString()=="[object Branded]");
}
TEST_CASE("Map Set keys use SameValueZero, insertion order and no coercion", "[runtime][collections]") {
    JSMap map;JSSet set;auto object=JSValue::object();
    object.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("must not coerce");}));
    const auto a=JSSymbol::create("key"),b=JSSymbol::create("key");
    Args keys{{},nullptr,false,true,-0.0,1,"1",std::numeric_limits<double>::quiet_NaN(),object,a,b};
    for(std::size_t i=0;i<keys.size();++i){REQUIRE(map.set(keys[i],i).sameValue(map.object()));REQUIRE(set.add(keys[i]).sameValue(set.object()));}
    REQUIRE(map.size()==keys.size());REQUIRE(set.size()==keys.size());
    map.set(0,99);set.add(0);map.set(std::nan("1"),77);set.add(std::nan("1"));
    REQUIRE(map.size()==keys.size());REQUIRE(map.get(-0.0).number()==99);REQUIRE(map.get(std::nan("2")).number()==77);
    auto iterator=map.keys();for(const auto& key:keys){const auto result=next(iterator);REQUIRE_FALSE(result.get("done").toBoolean());REQUIRE((result.get("value").strictEquals(key)||(std::isnan(result.get("value").toNumber())&&std::isnan(key.toNumber()))));}
    REQUIRE(next(iterator).get("done").toBoolean());
    auto zero=JSMap();zero.set(-0.0,-0.0);REQUIRE_FALSE(std::signbit(next(zero.keys()).get("value").number()));REQUIRE(std::signbit(zero.get(0).number()));
}
TEST_CASE("Collection live cursors survive deletion compaction reinsertion and clear", "[runtime][collections]") {
    for(bool isMap:{true,false}) {
        auto object=isMap?createNativeMap():createNativeSet();const char* add=isMap?"set":"add";
        for(unsigned i=0;i<200;++i)call(object,add,{i,i+1000});
        auto first=call(object,"keys"),second=call(object,"keys");
        for(unsigned i=0;i<25;++i)REQUIRE(next(first).get("value").number()==i);
        for(unsigned i=0;i<150;++i)call(object,"delete",{i});
        REQUIRE(next(first).get("value").number()==150);REQUIRE(next(second).get("value").number()==150);
        call(object,add,{150,3000});REQUIRE(next(first).get("value").number()==151);
        call(object,"delete",{150});call(object,add,{150,3001});
        for(unsigned i=152;i<200;++i)REQUIRE(next(first).get("value").number()==i);
        REQUIRE(next(first).get("value").number()==150);
        call(object,"clear");call(object,add,{999,999});
        REQUIRE(next(first).get("value").number()==999);REQUIRE(next(second).get("value").number()==999);
        REQUIRE(next(first).get("done").toBoolean());call(object,add,{1000,1000});
        REQUIRE(next(first).get("done").toBoolean());REQUIRE(next(second).get("value").number()==1000);
    }
}
TEST_CASE("forEach captures callback and keeps live insertion order under reentry", "[runtime][collections]") {
    auto object=createNativeMap();for(unsigned i=0;i<100;++i)call(object,"set",{i,i});
    auto seen=JSValue::array(),context=JSValue::object();context.set("marker",1);
    auto callback=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        REQUIRE(self.sameValue(c[1]));REQUIRE(args[0].sameValue(args[1]));REQUIRE(args[2].sameValue(c[0]));
        call(c[2],"push",{args[1]});
        if(args[1].number()==0) {for(unsigned i=1;i<90;++i)call(c[0],"delete",{i});call(c[0],"set",{101,101});}
        if(args[1].number()==90){call(c[0],"clear");call(c[0],"set",{102,102});}
        c[0].collectGarbage();return {};
    },{object,context,seen});
    REQUIRE(call(object,"forEach",{callback,context}).isUndefined());REQUIRE(seen.toJSON()->dump()=="[0,90,102]");
    REQUIRE_THROWS_AS(call(object,"forEach",{1}),JSTypeError);
}
TEST_CASE("Collection constructor dynamic iteration closes only entry and adder failures", "[runtime][collections][iterator]") {
    for(bool isMap:{true,false})for(unsigned failure=0;failure<8;++failure) {
        auto log=JSValue::array(),iterator=JSValue::object(),iterable=JSValue::object(),entry=JSValue::object();
        getter(entry,"0",+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"key"});if(c[1].number()==4)throw JSRangeError("entry");return "k";},{log,failure});
        getter(entry,"1",+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"value"});if(c[1].number()==5)throw JSRangeError("entry");return 8;},{log,failure});
        iterator.set("index",0);
        getter(iterator,"next",+[](const JSValue&,const Args&,const Args& c)->JSValue {
            call(c[0],"push",{"getNext"});return JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {
                call(c[0],"push",{"next"});if(c[1].number()==1)throw JSRangeError("next");
                auto iterator=self;const auto index=iterator.get("index").number();iterator.set("index",index+1);
                auto result=JSValue::object();
                getter(result,"done",+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"done"});if(c[1].number()==2)throw JSRangeError("done");return c[3].number()>0;},{c[0],c[1],c[2],index});
                getter(result,"value",+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"item"});if(c[1].number()==3)throw JSRangeError("item");return c[2];},c);
                return result;
            },c);
        },{log,failure,entry});
        getter(iterator,"return",+[](const JSValue&,const Args&,const Args& c)->JSValue {
            call(c[0],"push",{"getReturn"});return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"return"});throw JSTypeError("close failure");},c);
        },{log});
        iterable.set(JSSymbol::iterator(),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"iterator"});return c[1];},{log,iterator}));
        const auto base=isMap?nativeMapConstructor():nativeSetConstructor();auto target=JSValue::constructor(noop,{},"Derived",0,JSValue::ConstructorKind::Callable);
        auto prototype=JSValue::object(base.get("prototype"));target.set("prototype",prototype);
        getter(prototype,isMap?"set":"add",+[](const JSValue&,const Args&,const Args& c)->JSValue {
            call(c[0],"push",{"getAdder"});if(c[1].number()==7)return 7;
            return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{call(c[0],"push",{"add"});if(c[1].number()==6)throw JSRangeError("adder");return c[2].call(self,args);},c);
        },{log,failure,base.get("prototype").get(isMap?"set":"add")});
        const bool entryFailure=isMap&&(failure==4||failure==5),close=entryFailure||failure==6;
        if((failure>=1&&failure<=3)||entryFailure||failure==6) REQUIRE_THROWS_AS(base.construct({iterable},target),JSRangeError);
        else if(failure==7)REQUIRE_THROWS_AS(base.construct({iterable},target),JSTypeError);
        else REQUIRE(base.construct({iterable},target).get("size").number()==1);
        const auto text=log.toJSON()->dump();REQUIRE((text.find("getReturn")!=std::string::npos)==close);
        if(failure==7)REQUIRE(text=="[\"getAdder\"]");
    }
}
TEST_CASE("Array string iterators have replaceable protocol, live length and sticky exhaustion", "[runtime][iterator]") {
    auto array=tuple({1,2}),iterator=call(array,"values");
    REQUIRE(array.get(JSSymbol::iterator()).sameValue(array.get("values")));
    REQUIRE(next(iterator).get("value").number()==1);array.set("2",3);array.erase("1");
    REQUIRE(next(iterator).get("value").isUndefined());REQUIRE(next(iterator).get("value").number()==3);
    REQUIRE(next(iterator).get("done").toBoolean());array.set("3",4);REQUIRE(next(iterator).get("done").toBoolean());
    auto set=nativeSetConstructor().construct({"A\xF0\x9F\x98\x80"});REQUIRE(set.get("size").number()==2);
    auto record=getJSIterator("\xED\xA0\x80" "a");REQUIRE(stepJSIterator(record)->string()=="\xED\xA0\x80");REQUIRE(stepJSIterator(record)->string()=="a");REQUIRE_FALSE(stepJSIterator(record));
    array.set(JSSymbol::iterator(),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return getJSIterator("xy").iterator;}));
    REQUIRE(nativeSetConstructor().construct({array}).get("size").number()==2);
    auto values=nativeStringPrototype().get(JSSymbol::iterator());auto proto=nativeStringPrototype();
    proto.set(JSSymbol::iterator(),nullptr);REQUIRE_THROWS_AS(getJSIterator("x"),JSTypeError);proto.set(JSSymbol::iterator(),values);
}
TEST_CASE("Collection and iterator prototypes enforce brands and metadata", "[runtime][collections][iterator]") {
    for(bool isMap:{true,false}) {
        const auto ctor=isMap?nativeMapConstructor():nativeSetConstructor(),proto=ctor.get("prototype");
        REQUIRE(ctor.get("length").number()==0);REQUIRE_THROWS_AS(ctor.call(),JSTypeError);REQUIRE_THROWS_AS(proto.get("size"),JSTypeError);
        REQUIRE(ctor.get(JSSymbol::species()).sameValue(ctor));REQUIRE(proto.get(JSSymbol::toStringTag()).string()==(isMap?"Map":"Set"));
        auto object=ctor.construct();REQUIRE(call(object,"toString").string()==(isMap?"[object Map]":"[object Set]"));
        REQUIRE(proto.get(JSSymbol::iterator()).sameValue(proto.get(isMap?"entries":"values")));
        if(!isMap)REQUIRE(proto.get("keys").sameValue(proto.get("values")));
        for(const auto* name:{"has","delete","clear","keys","values","entries","forEach"}) {
            auto method=proto.get(name);REQUIRE_FALSE(method.isConstructible());REQUIRE_FALSE(*proto.getOwnPropertyDescriptor(name)->enumerable);
            REQUIRE_THROWS_AS(method.call(JSValue::object()),JSTypeError);
        }
        auto iterator=call(object,"entries");REQUIRE(iterator.get(JSSymbol::iterator()).call(iterator).sameValue(iterator));
        auto other=isMap?JSSet().values():JSMap().values();REQUIRE_THROWS_AS(iterator.get("next").call(other),JSTypeError);
        REQUIRE(iterator.ownPropertyKeys().empty());REQUIRE(object.ownPropertyKeys().empty());
    }
}
TEST_CASE("Heap traces collection cycles, symbol keys and live iterators without rooting tombstones", "[runtime][collections][symbol][gc]") {
    auto witness=JSValue::object();JSWeakValue weakMap,weakKey,weakValue,weakSymbol;
    {JSMap map;auto key=JSValue::object(),value=JSValue::object(),symbol=JSSymbol::create("key");
     weakMap=JSWeakValue(map.object());weakKey=JSWeakValue(key);weakValue=JSWeakValue(value);weakSymbol=JSWeakValue(symbol);
     map.set(key,value);map.set(symbol,map.object());key.set("map",map.object());value.set(symbol,key);}
    witness.collectGarbage();REQUIRE(weakMap.expired());REQUIRE(weakKey.expired());REQUIRE(weakValue.expired());REQUIRE(weakSymbol.expired());
    auto holder=JSValue::object();{auto symbol=JSSymbol::create();weakSymbol=JSWeakValue(symbol);holder.set(symbol,1);}
    witness.collectGarbage();REQUIRE_FALSE(weakSymbol.expired());holder={};witness.collectGarbage();REQUIRE(weakSymbol.expired());
    JSValue iterator;
    {JSMap map;auto key=JSValue::object(),value=JSValue::object();weakMap=JSWeakValue(map.object());weakKey=JSWeakValue(key);weakValue=JSWeakValue(value);
     map.set(key,value);iterator=map.entries();map.remove(key);}
    witness.collectGarbage();REQUIRE(weakMap.expired());REQUIRE(weakKey.expired());REQUIRE(weakValue.expired());
    REQUIRE(next(iterator).get("done").toBoolean());witness.collectGarbage();REQUIRE(weakMap.expired());
    {JSSet set;auto key=JSValue::object();weakMap=JSWeakValue(set.object());weakKey=JSWeakValue(key);set.add(key);iterator=set.values();}
    witness.collectGarbage();REQUIRE(weakMap.expired());REQUIRE_FALSE(weakKey.expired());
    next(iterator);next(iterator);witness.collectGarbage();REQUIRE(weakMap.expired());REQUIRE(weakKey.expired());
}
TEST_CASE("Collection wrapper and symbol keys reject foreign thread references", "[runtime][collections][symbol][thread]") {
    JSMap map;JSSet set;const auto key=JSSymbol::create("key");auto foreign=JSValue::object();bool rejected[20]={};
    std::thread worker([&]{
        try{map.size();}catch(const JSTypeError&){rejected[0]=true;}
        try{JSMap copy(map);}catch(const JSTypeError&){rejected[1]=true;}
        try{set.values();}catch(const JSTypeError&){rejected[2]=true;}
        try{JSMap local;local.set(key,1);}catch(const JSTypeError&){rejected[3]=true;}
        try{JSValue::object().set(key,1);}catch(const JSTypeError&){rejected[4]=true;}
        try{key.symbolDescription();}catch(const JSTypeError&){rejected[5]=true;}
        try{map=JSMap();}catch(const JSTypeError&){rejected[6]=true;}
        try{set=JSSet();}catch(const JSTypeError&){rejected[7]=true;}
        try{foreign.ownPropertyKeys();}catch(const JSTypeError&){rejected[8]=true;}
        try{foreign.get(JSValue("key"));}catch(const JSTypeError&){rejected[9]=true;}
        try{foreign.has(JSValue("key"));}catch(const JSTypeError&){rejected[10]=true;}
        try{foreign.hasOwn(JSValue("key"));}catch(const JSTypeError&){rejected[11]=true;}
        try{foreign.getOwnPropertyDescriptor(JSValue("key"));}catch(const JSTypeError&){rejected[12]=true;}
        try{foreign.erase(JSValue("key"));}catch(const JSTypeError&){rejected[13]=true;}
        try{foreign.set(JSValue("key"),1);}catch(const JSTypeError&){rejected[14]=true;}
        try{foreign.defineOwnProperty(JSValue("key"),data(1));}catch(const JSTypeError&){rejected[15]=true;}
        try{JSValue::object().get(JSValue("key"),foreign);}catch(const JSTypeError&){rejected[16]=true;}
        try{key.toPrimitive();}catch(const JSTypeError&){rejected[17]=true;}
        try{key.toJSON();}catch(const JSTypeError&){rejected[18]=true;}
        try{key.trace();}catch(const JSTypeError&){rejected[19]=true;}
    });worker.join();for(bool result:rejected)REQUIRE(result);
}
TEST_CASE("forEach and shared backing retain only reachable managed edges", "[runtime][collections][gc]") {
    auto state=JSValue::object(),witness=JSValue::object();JSWeakValue receiver;
    {JSMap map;map.set(1,2);state.set("map",map.object());receiver=JSWeakValue(map.object());}
    const auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        auto state=captures[0];state.erase("map");state.collectGarbage();
        REQUIRE(isNativeMap(args[2]));REQUIRE(args[2].get("size").number()==1);
        state.set("escaped",args[2]);return {};
    },{state});
    call(state.get("map"),"forEach",{callback});witness.collectGarbage();REQUIRE_FALSE(receiver.expired());
    state.erase("escaped");witness.collectGarbage();REQUIRE(receiver.expired());
    JSWeakValue weakIterator,weakKey;
    {JSMap map;auto iterator=map.entries(),key=JSValue::object();weakIterator=JSWeakValue(iterator);weakKey=JSWeakValue(key);receiver=JSWeakValue(map.object());
     key.set("map",map.object());map.set(iterator,key);}
    witness.collectGarbage();REQUIRE(receiver.expired());REQUIRE(weakIterator.expired());REQUIRE(weakKey.expired());
}

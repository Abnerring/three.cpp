#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/NativeObject.h"
#include "core/NativeArray.h"
#include <cmath>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values) { auto out=JSValue::array();for(const auto& value:values)out.get("push").call(out,{value});return out; }
JSValue call(const JSValue& object,const char* name,const Args& args={}) { return object.get(name).call(object,args); }
}
TEST_CASE("Float16 attribute runtime has canonical derived identity independent of flags", "[Float16Runtime]") {
    auto native=std::make_shared<Float16BufferAttribute>(std::vector<std::uint16_t>{15360,49152},1);
    auto value=nativeAttribute(native);const auto constructor=nativeFloat16BufferAttributeConstructor();
    REQUIRE(constructor.sameValue(nativeFloat16BufferAttributeConstructor()));
    REQUIRE(constructor.getPrototype().sameValue(nativeBufferAttributeConstructor()));
    REQUIRE(value.getPrototype().sameValue(constructor.get("prototype")));
    REQUIRE(instanceOfFloat16BufferAttribute(value));
    value.set("isFloat16BufferAttribute",false);REQUIRE(instanceOfFloat16BufferAttribute(value));
    REQUIRE(call(value,"getX",{0}).number()==1);REQUIRE(call(value,"getX",{1}).number()==-2);
    auto ordinary=nativeAttribute(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{15360},1));
    ordinary.set("isFloat16BufferAttribute",true);REQUIRE_FALSE(instanceOfFloat16BufferAttribute(ordinary));
    auto fake=JSValue::object(constructor.get("prototype"));REQUIRE(instanceOfFloat16BufferAttribute(fake));
    value.setPrototype(nativeBufferAttributeConstructor().get("prototype"));REQUIRE_FALSE(instanceOfFloat16BufferAttribute(value));
}
TEST_CASE("Managed Float16 construction retains backing copy and native half semantics", "[Float16Runtime]") {
    const auto constructor=nativeFloat16BufferAttributeConstructor();const auto input=NativeArray_detail::constructor(3).construct({array({15360,49152})});
    auto value=constructor.construct({input,1});
    REQUIRE(value.get("normalized").strictEquals(false));REQUIRE_FALSE(value.get("array").sameValue(input));
    REQUIRE(call(value,"getX",{0}).number()==1);REQUIRE(call(value,"getX",{1}).number()==-2);
    auto native=std::dynamic_pointer_cast<Float16BufferAttribute>(bufferAttributeRuntimeValue(value));
    REQUIRE(native);REQUIRE(native->getX(0)==1);REQUIRE(native->getX(1)==-2);
    REQUIRE(call(value,"setX",{0,.5}).sameValue(value));REQUIRE(value.get("array").get("0").number()==14336);
    REQUIRE(input.get("0").number()==15360);
    const auto clone=call(value,"clone");REQUIRE(instanceOfFloat16BufferAttribute(clone));REQUIRE(call(clone,"getX",{0}).number()==.5);
    value.collectGarbage();REQUIRE(value.getPrototype().sameValue(constructor.get("prototype")));
}
TEST_CASE("Float16 runtime respects newTarget and the live super constructor", "[Float16Runtime]") {
    auto constructor=nativeFloat16BufferAttributeConstructor();const auto super=constructor.getPrototype();
    auto subclass=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Derived",0);
    subclass.get("prototype").setPrototype(constructor.get("prototype"));
    const auto value=constructor.construct({array({15360}),1},subclass);
    REQUIRE(value.getPrototype().sameValue(subclass.get("prototype")));REQUIRE(instanceOfFloat16BufferAttribute(value));
    auto events=array({});auto replacement=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue {
        captures[0].get("push").call(captures[0],{args[0].get("0"),args[1],args[2]});return receiver;
    },{events},"Replacement",3);
    constructor.setPrototype(replacement);
    auto result=constructor.construct({array({65537}),"size","normalized"});
    constructor.setPrototype(super);
    REQUIRE(result.get("isFloat16BufferAttribute").strictEquals(true));
    REQUIRE(events.get("0").number()==1);REQUIRE(events.get("1").strictEquals("size"));REQUIRE(events.get("2").strictEquals("normalized"));
}
TEST_CASE("Float16 imported constructor survives public constructor replacement and hasInstance changes", "[Float16Runtime]") {
    auto constructor=nativeFloat16BufferAttributeConstructor(),prototype=constructor.get("prototype");
    prototype.set("constructor",nullptr);REQUIRE(nativeFloat16BufferAttributeConstructor().sameValue(constructor));
    auto value=constructor.construct({array({15360}),1});REQUIRE(instanceOfFloat16BufferAttribute(value));
    prototype.set("constructor",constructor);
    JSPropertyDescriptor hasInstance;hasInstance.value=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return false;});hasInstance.configurable=true;
    constructor.defineProperty(JSValue::wellKnownSymbol("hasInstance"),hasInstance);REQUIRE_FALSE(instanceOfFloat16BufferAttribute(value));
    constructor.erase(JSValue::wellKnownSymbol("hasInstance"));REQUIRE(instanceOfFloat16BufferAttribute(value));
}
TEST_CASE("Typed array construction consumes iterable before converting elements", "[Float16Runtime]") {
    for(std::size_t kind=0;kind<9;++kind) {
        auto events=array({}),state=JSValue::object();state.set("index",0);auto object=JSValue::object();
        JSPropertyDescriptor length;length.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSTypeError("iterable length must not be read");});
        object.defineProperty("length",length);
        object.set(JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {
            auto iterator=JSValue::object();iterator.set("next",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {
                auto state=captures[0];const auto index=state.get("index").number();state.set("index",index+1);
                captures[1].get("push").call(captures[1],{"next"});auto step=JSValue::object();step.set("done",index==2);
                if(index<2){auto element=JSValue::object();element.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {
                    captures[0].get("push").call(captures[0],{"convert"});return captures[1];
                },{captures[1],index+1}));step.set("value",element);}return step;
            },captures));return iterator;
        },{state,events}));
        const auto result=NativeArray_detail::constructor(kind).construct({object});
        REQUIRE(result.get("0").number()==1);REQUIRE(result.get("1").number()==2);
        REQUIRE(events.get("length").number()==5);REQUIRE(events.get("0").strictEquals("next"));
        REQUIRE(events.get("2").strictEquals("next"));REQUIRE(events.get("3").strictEquals("convert"));
    }
}
TEST_CASE("Managed Float16 native bridge preserves source descriptors and managed identity", "[Float16Runtime][ManagedBridge]") {
    const auto constructor=nativeFloat16BufferAttributeConstructor();auto value=constructor.construct({array({15360,16384}),1});
    const auto arrayBefore=value.get("array");const auto names=value.ownPropertyNames();
    REQUIRE(value.getOwnPropertyDescriptor("array")->value.has_value());
    auto restored=std::dynamic_pointer_cast<Float16BufferAttribute>(bufferAttributeRuntimeValue(value));
    REQUIRE(restored);REQUIRE(nativeAttribute(restored).sameValue(value));REQUIRE(value.ownPropertyNames()==names);
    REQUIRE(value.getOwnPropertyDescriptor("array")->value->sameValue(arrayBefore));
    REQUIRE(call(value,"setX",{1,.5}).sameValue(value));REQUIRE(restored->getX(1)==.5);
    auto replacement=constructor.construct({array({16896}),1});call(value,"copy",{replacement});
    REQUIRE(bufferAttributeRuntimeValue(value)==restored);REQUIRE(restored->count==1);REQUIRE(restored->getX(0)==3);
}
TEST_CASE("Managed Float16 bridge keeps external native owners and collects retired cycles", "[Float16Runtime][ManagedBridge][GC]") {
    const auto constructor=nativeFloat16BufferAttributeConstructor();JSWeakValue weak;
    {
        auto value=constructor.construct({array({15360}),1});weak=JSWeakValue(value);
        auto native=std::dynamic_pointer_cast<Float16BufferAttribute>(bufferAttributeRuntimeValue(value));
        REQUIRE(native);value={};constructor.collectGarbage();REQUIRE_FALSE(weak.expired());
        REQUIRE(native->getX(0)==1);REQUIRE(nativeAttribute(native).instanceOf(constructor));
    }
    constructor.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Managed internal slots preserve descriptors and collect ordinary object cycles", "[ManagedBridge][GC]") {
    auto keeper=JSValue::object();JSWeakValue firstWeak,secondWeak;
    {
        auto first=JSValue::object(),second=JSValue::object();firstWeak=JSWeakValue(first);secondWeak=JSWeakValue(second);
        JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("private slots must not invoke property access");});descriptor.configurable=true;
        first.defineProperty("private",descriptor);const auto names=first.ownPropertyNames();
        first.preventExtensions();first.setInternalSlot("private",second);second.setInternalSlot("back",first);
        REQUIRE(first.getInternalSlot("private").sameValue(second));REQUIRE(second.getInternalSlot("back").sameValue(first));
        REQUIRE(first.ownPropertyNames()==names);REQUIRE(first.getOwnPropertyDescriptor("private")->getter->isCallable());
        REQUIRE_THROWS_AS(first.getHostSlot("private"),JSTypeError);
        REQUIRE_THROWS_AS(JSValue(0).getInternalSlot("private"),JSTypeError);
    }
    keeper.collectGarbage();REQUIRE(firstWeak.expired());REQUIRE(secondWeak.expired());
}
TEST_CASE("Managed native bridge ignores public Float16 hasInstance hooks", "[Float16Runtime][ManagedBridge]") {
    auto constructor=nativeFloat16BufferAttributeConstructor();const auto symbol=JSValue::wellKnownSymbol("hasInstance");
    const auto prototype=constructor.getOwnPropertyDescriptor("prototype").value();
    REQUIRE_FALSE(prototype.writable.value_or(true));REQUIRE_FALSE(prototype.configurable.value_or(true));
    for(int mode=0;mode<3;++mode){
        auto value=(mode==1?nativeBufferAttributeConstructor():constructor).construct({mode==1?NativeArray_detail::constructor(3).construct({array({15360})}):array({15360}),1});
        auto events=array({});const auto original=constructor.getOwnPropertyDescriptor(symbol);JSPropertyDescriptor d;d.configurable=true;
        if(mode==2)d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{c[0].get("push").call(c[0],{"get"});throw JSTypeError("native bridge must not read public hasInstance");},{events},"get hasInstance",0);
        else d.value=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{c[0].get("push").call(c[0],{"call"});return c[1];},{events,mode==1},"hasInstance",1);
        constructor.defineProperty(symbol,d);std::shared_ptr<BufferAttribute> native;
        try{native=bufferAttributeRuntimeValue(value);}catch(...){if(original)constructor.defineProperty(symbol,*original);else constructor.erase(symbol);throw;}
        if(original)constructor.defineProperty(symbol,*original);else constructor.erase(symbol);
        REQUIRE(events.length()==0);REQUIRE(nativeAttribute(native).sameValue(value));
        REQUIRE(bool(std::dynamic_pointer_cast<Float16BufferAttribute>(native))==(mode!=1));
        REQUIRE(native->getX(0)==(mode==1?15360:1));
    }
}

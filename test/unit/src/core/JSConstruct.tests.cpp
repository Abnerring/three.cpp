#include "catch.hpp"
#include "core/JSValue.h"
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue empty(const JSValue&,const Args&,const Args&){return {};}
JSValue returnCapture(const JSValue& receiver,const Args& args,const Args& captures){
    auto result=receiver;result.set("argument",args.empty()?JSValue():args[0]);return captures[0];
}
}

TEST_CASE("Class constructors have standard metadata and a managed prototype backlink", "[core][construct]") {
    const auto constructor=JSValue::constructor(empty,{},"Example",2);
    REQUIRE(constructor.isCallable());REQUIRE(constructor.isConstructible());
    REQUIRE(constructor.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});
    for(const auto* name:{"name","length"}) {
        const auto descriptor=constructor.getOwnPropertyDescriptor(name);
        REQUIRE_FALSE(*descriptor->writable);REQUIRE_FALSE(*descriptor->enumerable);REQUIRE(*descriptor->configurable);
    }
    REQUIRE(constructor.get("length").number()==2);REQUIRE(constructor.get("name").string()=="Example");
    const auto descriptor=constructor.getOwnPropertyDescriptor("prototype");
    REQUIRE_FALSE(*descriptor->writable);REQUIRE_FALSE(*descriptor->enumerable);REQUIRE_FALSE(*descriptor->configurable);
    const auto prototype=constructor.get("prototype");
    REQUIRE(prototype.ownPropertyNames()==std::vector<std::string>{"constructor"});
    REQUIRE(prototype.get("constructor").sameValue(constructor));
    const auto backlink=prototype.getOwnPropertyDescriptor("constructor");
    REQUIRE(*backlink->writable);REQUIRE_FALSE(*backlink->enumerable);REQUIRE(*backlink->configurable);
    REQUIRE_THROWS_AS(constructor.call(),JSTypeError);
    REQUIRE_FALSE(JSValue::function(empty).isConstructible());
    REQUIRE_THROWS_AS(JSValue::function(empty).construct(),JSTypeError);
    REQUIRE_THROWS_AS(JSValue::constructor(nullptr),JSTypeError);
}

TEST_CASE("Construct ignores primitive returns and uses object returns by identity", "[core][construct]") {
    for(const auto& returned:Args{JSValue{},nullptr,false,3,"text",std::numeric_limits<double>::quiet_NaN(),JSValue::object(),JSValue::array(),JSValue::function(empty)}) {
        const auto constructor=JSValue::constructor(returnCapture,{returned});
        const auto result=constructor.construct({42});
        if(returned.isObject())REQUIRE(result.sameValue(returned));
        else {
            REQUIRE(result.isObject());REQUIRE(result.get("argument").number()==42);
            REQUIRE(result.getPrototype().sameValue(constructor.get("prototype")));
        }
    }
}

TEST_CASE("Explicit callable constructors keep call and construct semantics distinct", "[core][construct]") {
    auto constructor=JSValue::constructor(returnCapture,{17},"Example",1,JSValue::ConstructorKind::Callable);
    REQUIRE(*constructor.getOwnPropertyDescriptor("prototype")->writable);
    const auto receiver=JSValue::object();
    REQUIRE(constructor.call(receiver,{9}).number()==17);REQUIRE(receiver.get("argument").number()==9);
    const auto instance=constructor.construct({4});REQUIRE(instance.get("argument").number()==4);
    REQUIRE_FALSE(instance.sameValue(receiver));
    for(const auto& prototype:Args{JSValue{},nullptr,false,4,"primitive"}) {
        constructor.set("prototype",prototype);
        REQUIRE(constructor.construct().getPrototype().sameValue(JSValue::object().getPrototype()));
    }
    const auto prototype=JSValue::array();constructor.set("prototype",prototype);
    REQUIRE(constructor.construct().getPrototype().sameValue(prototype));
}

TEST_CASE("Reflect construction chooses newTarget prototype without calling newTarget", "[core][construct]") {
    auto state=JSValue::object();state.set("calls",0);
    const auto base=JSValue::constructor(returnCapture,{JSValue{}},"Base");
    const auto target=JSValue::constructor([](const JSValue&,const Args&,const Args& captures){
        auto state=captures[0];state.set("calls",state.get("calls").number()+1);return JSValue();
    },{state},"Target");
    const auto value=base.construct({7},target);
    REQUIRE(value.getPrototype().sameValue(target.get("prototype")));
    REQUIRE(value.get("constructor").sameValue(target));REQUIRE(value.get("argument").number()==7);
    REQUIRE(state.get("calls").number()==0);
}

TEST_CASE("Constructibility validation precedes observing a prototype getter", "[core][construct]") {
    auto log=JSValue::array();const auto constructor=JSValue::constructor(empty);
    for(auto target:Args{JSValue::object(),JSValue::function(empty)}) {
        JSPropertyDescriptor property;property.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures){
            auto log=captures[0];log.set(std::to_string(log.length()),"getter");throw JSTypeError("getter");return JSValue();
        },{log});
        target.defineProperty("prototype",property);
        REQUIRE_THROWS_AS(constructor.construct({},target),JSTypeError);
        REQUIRE_THROWS_AS(target.construct({},constructor),JSTypeError);
        REQUIRE(log.length()==0);
    }
}

TEST_CASE("Construction roots detached arguments captures and receivers during collection", "[core][construct]") {
    auto anchor=JSValue::object();auto holder=JSValue::object();std::weak_ptr<int> weak;
    {
        auto native=std::make_shared<int>(7);weak=native;
        auto captured=JSValue::hostObject(native,native.get(),"construct-capture");
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args& captures){
            auto holder=captures[0];holder.erase("constructor");holder.erase("argument");
            self.collectGarbage();auto result=self;
            result.set("capture",captures[1]);result.set("argument",args[0]);return JSValue();
        },{holder,captured});
        holder.set("constructor",constructor);holder.set("argument",JSValue::object());
    }
    auto result=holder.get("constructor").construct({holder.get("argument")});
    REQUIRE_FALSE(weak.expired());REQUIRE(result.get("argument").isObject());
    REQUIRE(*std::static_pointer_cast<int>(result.get("capture").hostPayload("construct-capture"))==7);
    result=JSValue();anchor.collectGarbage();REQUIRE(weak.expired());
}

TEST_CASE("Constructor prototype capture cycles collect without external handles", "[core][construct]") {
    auto anchor=JSValue::object();std::weak_ptr<int> weak;
    {
        auto native=std::make_shared<int>(5);weak=native;
        auto holder=JSValue::object();auto captured=JSValue::hostObject(native,native.get(),"constructor-cycle");
        auto constructor=JSValue::constructor(empty,{holder,captured});
        holder.set("constructor",constructor);constructor.get("prototype").get("constructor").construct();
        anchor.collectGarbage();REQUIRE_FALSE(weak.expired());
    }
    anchor.collectGarbage();REQUIRE(weak.expired());
}

TEST_CASE("Constructor exceptions preserve escaped receivers and support reentry", "[core][construct]") {
    auto state=JSValue::object();auto log=JSValue::array();state.set("depth",0);
    auto constructor=JSValue::constructor([](const JSValue& self,const Args&,const Args& captures){
        auto state=captures[0],log=captures[1];const auto depth=state.get("depth").number();
        log.set(std::to_string(log.length()),depth);state.set("escaped",self);
        if(depth==0){state.set("depth",1);state.get("constructor").construct();}
        throw JSRangeError("body");return JSValue();
    },{state,log});
    state.set("constructor",constructor);
    REQUIRE_THROWS_AS(constructor.construct(),JSRangeError);
    REQUIRE(log.length()==2);REQUIRE(log.get("0").number()==0);REQUIRE(log.get("1").number()==1);
    REQUIRE(state.get("escaped").getPrototype().sameValue(constructor.get("prototype")));
    state.erase("constructor");constructor=JSValue();state.collectGarbage();
    REQUIRE(state.get("escaped").get("constructor").isConstructible());
}

TEST_CASE("Object and Array intrinsic construction respect replacement newTarget", "[core][construct]") {
    const auto objectConstructor=JSValue::object().get("constructor");
    const auto arrayConstructor=JSValue::array().get("constructor");
    REQUIRE(objectConstructor.isConstructible());REQUIRE(arrayConstructor.isConstructible());
    const auto original=JSValue::object();REQUIRE(objectConstructor.construct({original}).sameValue(original));
    auto target=JSValue::constructor(empty,{},"Target",0,JSValue::ConstructorKind::Callable);
    const auto object=objectConstructor.construct({original},target);
    REQUIRE_FALSE(object.sameValue(original));REQUIRE(object.getPrototype().sameValue(target.get("prototype")));
    auto array=arrayConstructor.construct({3},target);
    REQUIRE(array.type()==JSValue::Type::Array);REQUIRE(array.length()==3);REQUIRE_FALSE(array.hasOwn("0"));
    REQUIRE(array.getPrototype().sameValue(target.get("prototype")));
    target.set("prototype",nullptr);
    REQUIRE(arrayConstructor.construct({},target).getPrototype().sameValue(JSValue::array().getPrototype()));
    REQUIRE_THROWS_AS(arrayConstructor.construct({-1},target),JSRangeError);
}

#include "catch.hpp"
#include "core/JSValue.h"
#include <exception>
#include <thread>
namespace {
using Args = std::vector<JSValue>;
JSValue filter() { return JSValue::array().get("filter"); }
void own(JSValue object, const JSValue& key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value=value; d.writable=d.enumerable=d.configurable=true;
    object.defineProperty(key,d);
}
}
TEST_CASE("Array filter and species expose standard method metadata", "[core][filter]") {
    auto array=JSValue::array(), method=filter();
    REQUIRE(method.get("name").strictEquals("filter"));
    REQUIRE(method.get("length").strictEquals(1));
    REQUIRE_FALSE(method.hasOwn("prototype"));
    REQUIRE_THROWS_AS(method.construct(),JSTypeError);
    const auto d=array.getPrototype().getOwnPropertyDescriptor("filter");
    REQUIRE(*d->writable); REQUIRE_FALSE(*d->enumerable); REQUIRE(*d->configurable);
    const auto constructor=array.get("constructor");
    const auto species=constructor.getOwnPropertyDescriptor(JSValue::wellKnownSymbol("species"));
    REQUIRE(species->getter.has_value()); REQUIRE(species->setter->isUndefined());
    REQUIRE_FALSE(*species->enumerable); REQUIRE(*species->configurable);
    const auto getter=*species->getter;
    REQUIRE(getter.get("name").strictEquals("get [Symbol.species]"));
    REQUIRE(getter.get("length").strictEquals(0));
    REQUIRE_FALSE(getter.isConstructible());
    auto other=JSValue::object();
    REQUIRE(getter.call(other).sameValue(other));
    REQUIRE(getter.call(17).strictEquals(17));
}
TEST_CASE("Filter roots the selected element across callback deletion and collection", "[core][filter][gc]") {
    auto array=JSValue::array(), value=JSValue::object();value.set("marker",42);array.set("0",value);
    JSWeakValue weak(value);value={};
    const auto callback=JSValue::function([](const JSValue&,const Args& args,const Args&) -> JSValue {
        auto input=args[2];input.erase("0");input.collectGarbage();return true;
    });
    auto result=filter().call(array,{callback});
    REQUIRE_FALSE(weak.expired()); REQUIRE(result.get("0").get("marker").strictEquals(42));
    result={};array.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Filter species writes own data properties and preserves partial failure", "[core][filter]") {
    auto input=JSValue::array();input.set("0",10);input.set("1",20);
    auto target=JSValue::object(), prototype=JSValue::object();target.setPrototype(prototype);
    JSPropertyDescriptor setter;setter.setter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {
        throw JSRangeError("Inherited setter must not be called");
    });setter.configurable=true;prototype.defineProperty("0",setter);
    JSPropertyDescriptor locked;locked.value=99;locked.writable=false;locked.configurable=false;
    target.defineProperty("1",locked);
    auto constructor=JSValue::object();own(constructor,JSValue::wellKnownSymbol("species"),
        JSValue::constructor([](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{target},"Species",1));
    input.set("constructor",constructor);
    auto callback=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return true;});
    REQUIRE_THROWS_AS(filter().call(input,{callback}),JSTypeError);
    REQUIRE(target.getOwnPropertyDescriptor("0")->value->strictEquals(10));
    REQUIRE(target.get("1").strictEquals(99));
    REQUIRE_FALSE(target.hasOwn("length"));
}
TEST_CASE("Array species intrinsic survives deletion until the heap ends", "[core][filter][gc]") {
    std::exception_ptr error;
    std::thread worker([&] {try {
        JSWeakValue weakGetter, weakControl;
        {
            auto realm=JSValue::object();
            {
                auto array=JSValue::array(), constructor=array.get("constructor");
                const auto getter=*constructor.getOwnPropertyDescriptor(JSValue::wellKnownSymbol("species"))->getter;
                weakGetter=JSWeakValue(getter);
                auto control=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};});
                weakControl=JSWeakValue(control);
                constructor.erase(JSValue::wellKnownSymbol("species"));
            }
            realm.collectGarbage();
            REQUIRE_FALSE(weakGetter.expired());REQUIRE(weakControl.expired());
        }
        REQUIRE(weakGetter.expired());
    } catch (...) {error=std::current_exception();}});
    worker.join();if(error)std::rethrow_exception(error);
}

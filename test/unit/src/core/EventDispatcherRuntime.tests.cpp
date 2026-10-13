#include "core/EventDispatcher.h"
#include "catch.hpp"
#include <thread>

namespace {using Args=std::vector<JSValue>;}
TEST_CASE("EventDispatcher requires new and honours alternate construction prototype", "[core][event-dispatcher]") {
    const auto prototype=EventDispatcher::runtimePrototype(),constructor=prototype.get("constructor");
    REQUIRE(constructor.isConstructible());REQUIRE_THROWS_AS(constructor.call(),JSTypeError);
    const auto instance=constructor.construct();REQUIRE(instance.getPrototype().sameValue(prototype));REQUIRE(instance.ownPropertyNames().empty());
    auto alternate=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Alternate",0,JSValue::ConstructorKind::Callable);
    auto custom=JSValue::object();alternate.set("prototype",custom);REQUIRE(constructor.construct({},alternate).getPrototype().sameValue(custom));
    alternate.set("prototype",nullptr);REQUIRE(constructor.construct({},alternate).getPrototype().sameValue(JSValue::object().getPrototype()));
}
TEST_CASE("EventDispatcher module roots preserve mutation without retaining the heap", "[core][event-dispatcher][lifetime]") {
    std::vector<bool> result;
    std::thread worker([&]{
        JSWeakValue oldConstructor,oldPrototype,markerWitness;std::weak_ptr<int> resourceWitness;
        {
            auto anchor=JSValue::object();
            {
                auto prototype=EventDispatcher::runtimePrototype(),constructor=prototype.get("constructor"),marker=JSValue::object();auto resource=std::make_shared<int>(1);resourceWitness=resource;
                marker.set("resource",JSValue::hostObject(resource,resource.get(),"EventDispatcher.test.resource"));
                prototype.set("marker",marker);prototype.set("constructor",99);constructor.set("custom",21);
                oldConstructor=JSWeakValue(constructor);oldPrototype=JSWeakValue(prototype);markerWitness=JSWeakValue(marker);
            }
            anchor.collectGarbage();auto prototype=EventDispatcher::runtimePrototype();
            result.push_back(!oldConstructor.expired()&&!oldPrototype.expired()&&!markerWitness.expired());
            result.push_back(prototype.get("constructor").strictEquals(99)&&(oldConstructor.lock()&&oldConstructor.lock()->get("custom").strictEquals(21)));
            result.push_back(EventDispatcher::runtimeObject().getPrototype().sameValue(prototype));
        }
        result.push_back(oldConstructor.expired()&&oldPrototype.expired()&&markerWitness.expired()&&resourceWitness.expired());
        {
            auto prototype=EventDispatcher::runtimePrototype();result.push_back(prototype.get("marker").isUndefined()&&prototype.get("constructor").isConstructible());
        }
    });worker.join();REQUIRE(result==std::vector<bool>{true,true,true,true,true});
}
TEST_CASE("EventDispatcher cache weak handles reject foreign thread access", "[core][event-dispatcher]") {
    auto prototype=EventDispatcher::runtimePrototype();bool rejected=false;
    std::thread worker([&]{try{static_cast<void>(JSWeakValue(prototype));}catch(const JSTypeError&){rejected=true;}});worker.join();REQUIRE(rejected);
}

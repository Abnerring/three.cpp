#include "catch.hpp"
#include "core/JSPromise.h"

TEST_CASE("Promise jobs preserve FIFO order and defer reactions", "[runtime][promise]") {
    auto value=JSValue::array();const auto deferred=createNativePromiseCapability();
    deferred.promise.get("then").call(deferred.promise,{JSValue::function(+[](const JSValue&,const auto& args,const auto& captures)->JSValue {
        auto events=captures[0];events.set("0",args[0]);return {};
    },{value})});
    deferred.resolve.call({}, {42});
    REQUIRE(value.length()==0);
    REQUIRE(hasPendingMicrotasks());
    REQUIRE(drainMicrotasks()==1);
    REQUIRE(value.get("0").number()==42);
    REQUIRE_FALSE(hasPendingMicrotasks());
}

TEST_CASE("Microtask checkpoint drains queued work after an exception and resists reentry", "[runtime][promise]") {
    auto value=JSValue::array();
    queueMicrotask(JSValue::function(+[](const JSValue&,const auto&,const auto&)->JSValue {throw JSThrownValue("failure");}));
    queueMicrotask(JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
        auto value=captures[0];value.set("0",drainMicrotasks());value.set("1",true);return {};
    },{value}));
    REQUIRE_THROWS_AS(drainMicrotasks(),JSThrownValue);
    REQUIRE(value.get("0").number()==0);
    REQUIRE(value.get("1").toBoolean());
    REQUIRE_FALSE(hasPendingMicrotasks());
}

TEST_CASE("Pending promise cycles collect while queued reactions retain native captures", "[runtime][promise][gc]") {
    auto constructor=nativePromiseConstructor();constructor.collectGarbage();
    std::weak_ptr<int> weak;
    {
        const auto owner=std::make_shared<int>(17);weak=owner;
        const auto payload=JSValue::hostObject(owner,owner.get(),"promise.test.owner");
        auto capability=createNativePromiseCapability();
        capability.promise.get("then").call(capability.promise,{JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
            return *std::static_pointer_cast<int>(captures[0].hostPayload("promise.test.owner"));
        },{payload})});
        capability.resolve.call({}, {0});
    }
    constructor.collectGarbage();REQUIRE_FALSE(weak.expired());
    drainMicrotasks();constructor.collectGarbage();REQUIRE(weak.expired());
    {
        const auto owner=std::make_shared<int>(19);weak=owner;
        auto capability=createNativePromiseCapability();
        auto callback=JSValue::function(+[](const JSValue&,const auto&,const auto&)->JSValue {return {};},
            {capability.promise,JSValue::hostObject(owner,owner.get(),"promise.test.owner")});
        capability.promise.get("then").call(capability.promise,{callback});
    }
    constructor.collectGarbage();REQUIRE(weak.expired());
}

#include "catch.hpp"
#include "core/JSFinalizationRegistry.h"
#include "core/JSWeakRef.h"
#include "core/JSJobs.h"
#include "core/NativeEventLoop.h"
#include <chrono>

namespace {
using Args = std::vector<JSValue>;
void appendFinalization(JSValue values, JSValue value) { values.get("push").call(values,{value}); }
JSValue finalizationLog(JSValue values, JSValue value) {
    return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        appendFinalization(c[0],c[1]);return {};
    },{values,value});
}
struct LoopContext { NativeEventLoop* loop; };
}

TEST_CASE("Finalization cleanup is a separate native host task with its microtask checkpoint", "[runtime][eventloop][finalization]") {
    drainMicrotasks(); drainFinalizationJobs();
    NativeEventLoop loop; auto events=JSValue::array();auto held=JSValue::object();
    auto registry=createNativeFinalizationRegistry(JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        appendFinalization(c[0],self.isUndefined());appendFinalization(c[0],a[0]);
        queueMicrotask(finalizationLog(c[0],"micro"));return {};
    },{events}));
    registry.get("register").call(registry,{JSValue::object(),held});
    registry.collectGarbage();
    REQUIRE(events.get("length").strictEquals(0));
    REQUIRE(loop.hasReadyTasks()); REQUIRE(loop.hasPendingTasks());
    REQUIRE(loop.waitForWork(std::chrono::milliseconds(0)));
    const auto result=loop.runOneTask();
    REQUIRE(result.ranTask()); REQUIRE(result.failures.empty());
    REQUIRE(events.get("0").strictEquals(true)); REQUIRE(events.get("1").sameValue(held));
    REQUIRE(events.get("2").strictEquals("micro"));
    REQUIRE_FALSE(loop.hasPendingTasks()); REQUIRE_FALSE(loop.runOneTask().ranTask());
}

TEST_CASE("Native callback GC schedules cleanup for a later task", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto events=JSValue::array();
    auto registry=createNativeFinalizationRegistry(finalizationLog(events,"cleanup"));
    registry.get("register").call(registry,{JSValue::object(),"held"});
    loop.postTask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        appendFinalization(c[0],"task");c[1].collectGarbage();
        appendFinalization(c[0],"after-gc");queueMicrotask(finalizationLog(c[0],"micro"));return {};
    },{events,registry}));
    REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(events.toJSON().value().dump()=="[\"task\",\"after-gc\",\"micro\"]");
    REQUIRE(loop.hasPendingTasks());
    REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(events.toJSON().value().dump()=="[\"task\",\"after-gc\",\"micro\",\"cleanup\"]");
}

TEST_CASE("A native task turn dispatches at most one registry cleanup job", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto events=JSValue::array();
    auto first=createNativeFinalizationRegistry(finalizationLog(events,"first"));
    auto second=createNativeFinalizationRegistry(finalizationLog(events,"second"));
    first.get("register").call(first,{JSValue::object(),0});
    second.get("register").call(second,{JSValue::object(),0});
    first.collectGarbage();
    REQUIRE(drainFinalizationJobs(0)==0);REQUIRE(events.get("length").strictEquals(0));
    REQUIRE(loop.runOneTask().ranTask());REQUIRE(events.get("length").strictEquals(1));
    REQUIRE(loop.hasPendingTasks());
    REQUIRE(loop.runOneTask().ranTask());REQUIRE(events.get("length").strictEquals(2));
    REQUIRE_FALSE(loop.hasPendingTasks());
    REQUIRE_FALSE(events.get("0").sameValue(events.get("1")));
}

TEST_CASE("A throwing cleanup consumes its cell and reports a native task failure", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto events=JSValue::array();
    auto registry=createNativeFinalizationRegistry(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        const bool first=c[0].get("length").strictEquals(0);appendFinalization(c[0],a[0]);
        if(first) {
            queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue {
                throw JSTypeError("cleanup microtask failure");
            }));
            throw JSTypeError("cleanup failure");
        }
        return {};
    },{events}));
    registry.get("register").call(registry,{JSValue::object(),"one"});
    registry.get("register").call(registry,{JSValue::object(),"two"});
    registry.collectGarbage();const auto failed=loop.runOneTask();
    REQUIRE(failed.ranTask());REQUIRE(failed.failures.size()==2);
    REQUIRE(failed.failures[0].phase==NativeEventLoop::ErrorPhase::Task);
    REQUIRE(failed.failures[1].phase==NativeEventLoop::ErrorPhase::AfterTask);
    REQUIRE(events.get("length").strictEquals(1)); REQUIRE(loop.hasPendingTasks());
    const auto next=loop.runOneTask();REQUIRE(next.ranTask());REQUIRE(next.failures.empty());
    REQUIRE(events.get("length").strictEquals(2));REQUIRE_FALSE(loop.hasPendingTasks());
    REQUIRE_FALSE(events.get("0").sameValue(events.get("1")));
}

TEST_CASE("Standalone cleanup callbacks cannot recursively pump or wait on native tasks", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto events=JSValue::array();
    const auto owner=std::make_shared<LoopContext>(LoopContext{&loop});
    const auto context=JSValue::hostObject(owner,owner.get(),"test.FinalizationLoop");
    auto registry=createNativeFinalizationRegistry(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        const auto loop=std::static_pointer_cast<LoopContext>(c[1].hostPayload("test.FinalizationLoop"))->loop;
        appendFinalization(c[0],loop->runOneTask().ranTask());
        appendFinalization(c[0],loop->waitForWork(std::chrono::milliseconds(0)));
        queueMicrotask(finalizationLog(c[0],"micro"));return {};
    },{events,context}));
    registry.get("register").call(registry,{JSValue::object(),0});registry.collectGarbage();
    loop.postTask(finalizationLog(events,"native"));
    REQUIRE(drainFinalizationJobs()==1);
    REQUIRE(events.toJSON().value().dump()=="[false,false,\"micro\"]");
    REQUIRE(loop.runOneTask().ranTask());REQUIRE(events.get("3").strictEquals("native"));
}

TEST_CASE("WeakRef keeps targets through a native task and all of its microtasks", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto state=JSValue::object();auto events=JSValue::array();
    loop.postTask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto target=JSValue::object();const auto weak=createNativeWeakRef(target);target={};
        auto state=c[0];state.set("weak",weak);state.collectGarbage();
        appendFinalization(c[1],weak.get("deref").call(weak).isObject());
        queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            c[0].collectGarbage();const auto weak=c[0].get("weak");
            appendFinalization(c[1],weak.get("deref").call(weak).isObject());return {};
        },c));return {};
    },{state,events}));
    REQUIRE(loop.runOneTask().failures.empty());
    REQUIRE(events.toJSON().value().dump()=="[true,true]");
    state.collectGarbage();const auto weak=state.get("weak");
    REQUIRE(weak.get("deref").call(weak).isUndefined());
}

TEST_CASE("Cleanup callbacks cannot end their host job with a nested microtask checkpoint", "[runtime][eventloop][finalization]") {
    NativeEventLoop loop;auto events=JSValue::array();
    auto registry=createNativeFinalizationRegistry(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto target=JSValue::object();const auto weak=createNativeWeakRef(target);target={};
        c[0].collectGarbage();appendFinalization(c[0],weak.get("deref").call(weak).isObject());
        queueMicrotask(finalizationLog(c[0],"micro"));
        appendFinalization(c[0],static_cast<double>(drainMicrotasks()));
        c[0].collectGarbage();appendFinalization(c[0],weak.get("deref").call(weak).isObject());
        return {};
    },{events}));
    registry.get("register").call(registry,{JSValue::object(),0});registry.collectGarbage();
    const auto result=loop.runOneTask();REQUIRE(result.ranTask());REQUIRE(result.failures.empty());
    REQUIRE(events.toJSON().value().dump()=="[true,0,true,\"micro\"]");
    REQUIRE_FALSE(hasPendingMicrotasks());
}

TEST_CASE("Native wait notices cleanup queued by its injected clock", "[runtime][eventloop][finalization]") {
    auto events=JSValue::array();auto registry=createNativeFinalizationRegistry(finalizationLog(events,"cleanup"));
    registry.get("register").call(registry,{JSValue::object(),0});
    NativeEventLoop loop([&registry] {registry.collectGarbage();return 0.0;});
    REQUIRE(loop.waitForWork(std::chrono::milliseconds(0))); REQUIRE(events.get("length").strictEquals(0));
    REQUIRE(loop.runOneTask().ranTask()); REQUIRE(events.get("0").strictEquals("cleanup"));
}

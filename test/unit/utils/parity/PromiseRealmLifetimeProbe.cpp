#include "core/JSPromise.h"
#include "core/JSGlobal.h"
#include "core/JSGenerator.h"
#include "core/JSHostPerformance.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Args=std::vector<JSValue>;
std::size_t retirements=0;
void require(bool condition,const char* message){if(!condition)throw std::logic_error(message);}
JSValue invoke(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
struct Observations{std::vector<std::string> events;};
JSValue observationCarrier(const std::shared_ptr<Observations>& values){return JSValue::hostObject(values,values.get(),"PromiseRealmObservations");}
void record(const JSValue& carrier,std::string text){std::static_pointer_cast<Observations>(carrier.hostPayload("PromiseRealmObservations"))->events.push_back(std::move(text));}
void collected(const JSWeakValue& ctor,const JSWeakValue& global){JSValue::nativeSafepoint();require(ctor.expired(),"Promise intrinsic strong cache prevented actual realm retirement");require(global.expired(),"Promise global prevented actual realm retirement");++retirements;}
void runRound(){
    // No JSValue output accumulator is kept across these actual realm drops.
    JSWeakValue ctorWeak,globalWeak;
    {
        const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor();
        require(global.get("Promise").sameValue(ctor),"Global Promise is not the actual private intrinsic");
        require(nativePromiseConstructor().sameValue(ctor),"Promise intrinsic identity changed while realm alive");
        ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
    }
    collected(ctorWeak,globalWeak);
    {
        JSValue heldThen;
        {const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);heldThen=ctor.get("prototype").get("then");}
        JSValue::nativeSafepoint();require(!ctorWeak.expired()&&!globalWeak.expired(),"Live Promise method failed to keep its actual realm");
        {const auto ctor=nativePromiseConstructor();require(ctorWeak.lock()->sameValue(ctor),"Weak Promise cache recreated a live intrinsic");}
        heldThen={};
    }
    collected(ctorWeak,globalWeak);
    {
        auto global=nativeGlobalObject();const auto ctor=nativePromiseConstructor();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
        global.set("Promise",17);require(nativePromiseConstructor().sameValue(ctor),"Mutable global Promise replaced private authority");
        const auto promise=resolveNativePromise(3);require(promise.getPrototype().sameValue(ctor.get("prototype")),"Internal resolve used public Promise replacement");
        global.erase("Promise");require(nativePromiseConstructor().sameValue(ctor),"Deleting public Promise replaced private authority");
    }
    collected(ctorWeak,globalWeak);
    {
        auto keeper=JSValue::object();const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);JSWeakValue pendingWeak;
        {auto cap=createNativePromiseCapability();pendingWeak=JSWeakValue(cap.promise);invoke(cap.promise,"then",{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{cap.promise})});}
        keeper.collectGarbage();require(pendingWeak.expired(),"Unreachable pending Promise reaction cycle retained itself");
    }
    collected(ctorWeak,globalWeak);
    {
        const auto observations=std::make_shared<Observations>();
        {
            const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
            const auto promise=resolveNativePromise(9),carrier=observationCarrier(observations);
            invoke(promise,"then",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
                require(c[1].sameValue(nativePromiseConstructor()),"Pending reaction lost private Promise identity");
                JSValue::nativeSafepoint();record(c[0],a[0].strictEquals(9)?"reaction":"wrong");
                require(drainMicrotasks()==0,"Reentrant Promise checkpoint drained recursively");return {};
            },{carrier,ctor})});
        }
        JSValue::nativeSafepoint();require(hasPendingMicrotasks()&&!ctorWeak.expired()&&!globalWeak.expired(),"Pending job queue failed to retain actual realm");
        drainMicrotasks();require(observations->events==std::vector<std::string>{"reaction"},"Promise queued reaction did not complete exactly once");
    }
    collected(ctorWeak,globalWeak);
    {
        const auto observations=std::make_shared<Observations>();
        {
            const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
            const auto cap=createNativePromiseCapability();const auto carrier=observationCarrier(observations);
            invoke(cap.promise,"then",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{record(c[0],a[0].strictEquals(12)?"host-reaction":"wrong");return {};},{carrier})});
            queueHostTask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{c[0].call({}, {12});return {};},{cap.resolve}));
        }
        JSValue::nativeSafepoint();require(hasPendingHostTasks()&&!ctorWeak.expired()&&!globalWeak.expired(),"Pending host task failed to retain Promise realm");
        require(drainHostTasks(1)==1,"Actual host task did not drain");require(observations->events==std::vector<std::string>{"host-reaction"},"Host task Promise reaction did not complete");
    }
    collected(ctorWeak,globalWeak);
    {
        const auto observations=std::make_shared<Observations>();
        {
            const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor(),carrier=observationCarrier(observations);ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
            auto thenable=JSValue::object();JSPropertyDescriptor descriptor;
            descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                require(nativePromiseConstructor().sameValue(c[0]),"Then getter recreated Promise intrinsic");JSValue::nativeSafepoint();record(c[1],"get then");
                return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{require(nativePromiseConstructor().sameValue(c[0]),"Then call recreated Promise intrinsic");record(c[1],"call then");a[0].call({}, {23});a[1].call({}, {24});return {};},{c[0],c[1]});
            },{ctor,carrier});thenable.defineProperty("then",descriptor);
            const auto promise=resolveNativePromise(thenable);
            invoke(promise,"then",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{record(c[0],a[0].strictEquals(23)?"fulfilled":"wrong");return {};},{carrier})});
        }
        require(!ctorWeak.expired(),"Scheduled thenable adoption lost its realm");drainMicrotasks();
        require(observations->events==std::vector<std::string>{"get then","call then","fulfilled"},"Actual thenable reentry order changed");
    }
    collected(ctorWeak,globalWeak);
    {
        const auto observations=std::make_shared<Observations>();
        {
            const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor(),carrier=observationCarrier(observations),reason=JSValue::object();ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
            const auto promise=createNativePromise(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{throw JSGeneratorThrown(c[0]);},{reason}));
            invoke(promise,"then",{{},JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{record(c[0],a[0].sameValue(c[1])?"raw-generator-rejection":"wrong");return {};},{carrier,reason})});
        }
        drainMicrotasks();require(observations->events==std::vector<std::string>{"raw-generator-rejection"},"GeneratorThrown arbitrary rejection identity regressed");
    }
    collected(ctorWeak,globalWeak);
    {
        const auto global=nativeGlobalObject(),ctor=nativePromiseConstructor(),performance=global.get("performance");ctorWeak=JSWeakValue(ctor);globalWeak=JSWeakValue(global);
        require(performance.sameValue(nativeHostPerformanceObject()),"Global performance is not actual native host object");
        const double first=invoke(performance,"now").toNumber(),second=invoke(performance,"now").toNumber();
        require(std::isfinite(first)&&first>=0&&std::isfinite(second)&&second>=first,"Host performance clock is not finite monotonic elapsed milliseconds");
        require(std::isfinite(performance.get("timeOrigin").toNumber())&&performance.get("timeOrigin").toNumber()>0,"Host performance timeOrigin is invalid");
        require(performance.get("now").get("name").strictEquals("now")&&performance.get("now").get("length").strictEquals(0),"Performance.now metadata differs");
    }
    collected(ctorWeak,globalWeak);
}
}
int main(){try{for(unsigned i=0;i<3;++i)runRound();std::cout<<"{\"rounds\":3,\"actualRealmRetirements\":"<<retirements<<",\"privateAuthority\":true,\"queuedJobsRetainUntilCompletion\":true,\"thenableReentry\":true,\"generatorThrownIdentity\":true,\"realMonotonicClock\":true}";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "core/JSHostPerformance.h"
#include <chrono>
#include <memory>

namespace {
struct ClockOrigin {
    std::chrono::steady_clock::time_point monotonic=std::chrono::steady_clock::now();
    double epochMilliseconds=std::chrono::duration<double,std::milli>(
        std::chrono::system_clock::now().time_since_epoch()).count();
};
constexpr auto clockTag="host.PerformanceClock";
}
JSValue nativeHostPerformanceObject() {
    static thread_local JSWeakValue weak;
    if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;
    const auto origin=std::make_shared<ClockOrigin>();
    auto object=JSValue::hostObject(origin,origin.get(),clockTag);
    auto now=JSValue::function(+[](const JSValue& self,const std::vector<JSValue>&,
                                 const std::vector<JSValue>&)->JSValue {
        const auto clock=std::static_pointer_cast<ClockOrigin>(self.hostPayload(clockTag));
        if(!clock)throw JSTypeError("Illegal invocation");
        return std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-clock->monotonic).count();
    },{},"now",0);
    JSPropertyDescriptor descriptor;descriptor.value=now;
    descriptor.writable=descriptor.configurable=true;object.defineProperty("now",descriptor);
    JSPropertyDescriptor timeOrigin;timeOrigin.value=origin->epochMilliseconds;
    timeOrigin.configurable=true;object.defineProperty("timeOrigin",timeOrigin);
    object.retainForHeapLifetime();weak=JSWeakValue(object);return object;
}

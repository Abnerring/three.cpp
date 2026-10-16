#include "catch.hpp"
#include "core/AbortController.h"
#include <exception>
#include <thread>
namespace {
using Checks = std::vector<std::pair<const char*, bool>>;
template <class F> void fresh(F run) {
    Checks values;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            run(values);
        } catch (...) {
            error = std::current_exception();
        }
    });
    worker.join();
    if (error)
        std::rethrow_exception(error);
    for (const auto& [name, value] : values) {
        INFO(name);
        REQUIRE(value);
    }
}
} // namespace
TEST_CASE("Abort prototypes retain realm identity without owning thread roots",
          "[abort][realm][gc]") {
    fresh([](Checks& c) {
        JSWeakValue controllerProto, signalProto, payload;
        c.emplace_back("entry empty", !JSValue::hasActiveHeap());
        {
            auto keeper = JSValue::object();
            {
                auto one = createNativeAbortController(), two = createNativeAbortController();
                auto cp = one.getPrototype(), sp = one.get("signal").getPrototype();
                controllerProto = JSWeakValue(cp);
                signalProto = JSWeakValue(sp);
                c.emplace_back("controller prototype shared", cp.sameValue(two.getPrototype()));
                c.emplace_back("signal prototype shared",
                               sp.sameValue(two.get("signal").getPrototype()));
                auto p = JSValue::object();
                p.set("self", p);
                payload = JSWeakValue(p);
                cp.set("payload", p);
            }
            keeper.collectGarbage();
            c.emplace_back("controller intrinsic", !controllerProto.expired());
            c.emplace_back("signal intrinsic", !signalProto.expired());
            c.emplace_back("prototype payload intrinsic", !payload.expired());
            auto next = createNativeAbortController();
            c.emplace_back("later instance same prototype",
                           next.getPrototype().sameValue(*controllerProto.lock()));
        }
        c.emplace_back("controller prototype releases", controllerProto.expired());
        c.emplace_back("signal prototype releases", signalProto.expired());
        c.emplace_back("payload releases", payload.expired());
        c.emplace_back("heap releases", !JSValue::hasActiveHeap());
    });
}
TEST_CASE("Abort controller and signal callback cycles remain collectible", "[abort][gc]") {
    fresh([](Checks& c) {
        auto keeper = JSValue::object();
        JSWeakValue controller, signal;
        {
            auto value = createNativeAbortController();
            auto sig = value.get("signal");
            controller = JSWeakValue(value);
            signal = JSWeakValue(sig);
            sig.set("onabort",
                    JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                         const std::vector<JSValue>& a) -> JSValue { return a[0]; },
                                      {value, sig}));
        }
        keeper.collectGarbage();
        c.emplace_back("controller cycle collected", controller.expired());
        c.emplace_back("signal cycle collected", signal.expired());
    });
}

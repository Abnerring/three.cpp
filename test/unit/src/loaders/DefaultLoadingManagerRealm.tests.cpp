#include "catch.hpp"
#include "loaders/Loader.h"
#include "loaders/LoadingManager.h"
#include <exception>
#include <thread>
#include <type_traits>
namespace {
using Args = std::vector<JSValue>;
using Checks = std::vector<std::pair<std::string, bool>>;
template <class F> void fresh(F run) {
    Checks checks;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            run(checks);
        } catch (...) {
            error = std::current_exception();
        }
    });
    worker.join();
    if (error)
        std::rethrow_exception(error);
    for (const auto& [name, value] : checks) {
        INFO(name);
        REQUIRE(value);
    }
}
template <class F> bool rejects(F run) {
    try {
        run();
    } catch (const JSTypeError&) {
        return true;
    }
    return false;
}
JSValue invoke(const JSValue& self, const char* name, const Args& args = {}) {
    return self.get(name).call(self, args);
}
} // namespace
TEST_CASE("DefaultLoadingManager is lazy and rejects rootless changes",
          "[loaders][default-manager][realm]") {
    static_assert(!std::is_copy_assignable_v<DefaultLoadingManagerFacade>);
    fresh([](Checks& c) {
        c.emplace_back("no eager heap", !JSValue::hasActiveHeap());
        DefaultLoadingManagerFacade facade;
        c.emplace_back("facade construction does not allocate", !JSValue::hasActiveHeap());
        c.emplace_back("onStart write", rejects([] { DefaultLoadingManager.onStart = nullptr; }));
        c.emplace_back("onLoad write", rejects([] { DefaultLoadingManager.onLoad = 7; }));
        c.emplace_back("onProgress write",
                       rejects([] { DefaultLoadingManager.onProgress = false; }));
        c.emplace_back("onError write", rejects([] { DefaultLoadingManager.onError = "bad"; }));
        c.emplace_back("controller write",
                       rejects([] { DefaultLoadingManager._abortController = nullptr; }));
        c.emplace_back("itemStart", rejects([] { DefaultLoadingManager.itemStart("a"); }));
        c.emplace_back("itemEnd", rejects([] { DefaultLoadingManager.itemEnd("a"); }));
        c.emplace_back("itemError", rejects([] { DefaultLoadingManager.itemError("a"); }));
        c.emplace_back("setURLModifier",
                       rejects([] { DefaultLoadingManager.setURLModifier(nullptr); }));
        c.emplace_back("addHandler", rejects([] { DefaultLoadingManager.addHandler(nullptr, 7); }));
        c.emplace_back("removeHandler",
                       rejects([] { DefaultLoadingManager.removeHandler(nullptr); }));
        c.emplace_back("abort", rejects([] { DefaultLoadingManager.abort(); }));
        c.emplace_back("failed operations allocate nothing", !JSValue::hasActiveHeap());
        c.emplace_back("URL read",
                       DefaultLoadingManager.resolveURL("e\xcc\x81").strictEquals("\xc3\xa9"));
        c.emplace_back("handler read", DefaultLoadingManager.getHandler("a").isNull());
        c.emplace_back("read does not leak heap", !JSValue::hasActiveHeap());
    });
}
TEST_CASE("DefaultLoadingManager is one authoritative heap singleton",
          "[loaders][default-manager][gc]") {
    fresh([](Checks& c) {
        JSWeakValue manager, prototype, constructor, callback;
        {
            auto keeper = JSValue::object();
            auto module = DefaultLoadingManager.object();
            manager = JSWeakValue(module);
            auto proto = LoadingManager::prototype();
            prototype = JSWeakValue(proto);
            constructor = JSWeakValue(proto.get("constructor"));
            c.emplace_back("same export", module.sameValue(nativeDefaultLoadingManager()));
            c.emplace_back("prototype identity", module.getPrototype().sameValue(proto));
            c.emplace_back("undefined callbacks", module.get("onStart").isUndefined() &&
                                                      module.get("onLoad").isUndefined() &&
                                                      module.get("onProgress").isUndefined() &&
                                                      module.get("onError").isUndefined());
            c.emplace_back("null controller default", module.get("_abortController").isNull());
            {
                auto fn = JSValue::function(
                    [](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                        auto state = captures[0];
                        state.set("correctReceiver", self.sameValue(captures[1]));
                        state.set("loaded", args.empty() ? JSValue{} : args[0]);
                        return {};
                    },
                    {keeper, module});
                callback = JSWeakValue(fn);
                DefaultLoadingManager.onStart = fn;
            }
            module = {};
            proto = {};
            keeper.collectGarbage();
            c.emplace_back("singleton intrinsic", !manager.expired());
            c.emplace_back("callback held", !callback.expired());
            DefaultLoadingManager.itemStart("a");
            c.emplace_back("callback receiver", keeper.get("correctReceiver").strictEquals(true));
            c.emplace_back("callback argument", keeper.get("loaded").strictEquals("a"));
            DefaultLoadingManager.onStart = JSValue{};
            keeper.collectGarbage();
            c.emplace_back("removed callback collected", callback.expired());
        }
        c.emplace_back("singleton dies with heap", manager.expired());
        c.emplace_back("prototype dies with heap", prototype.expired());
        c.emplace_back("constructor dies with heap", constructor.expired());
        c.emplace_back("heap gone", !JSValue::hasActiveHeap());
        {
            auto next = DefaultLoadingManager.object();
            c.emplace_back("next heap defaults", next.get("onStart").isUndefined() &&
                                                     next.get("_abortController").isNull());
        }
        c.emplace_back("next heap gone", !JSValue::hasActiveHeap());
    });
}
TEST_CASE("DefaultLoadingManager dispatches replaced methods and preserves wrapper returns",
          "[loaders][default-manager][protocol]") {
    fresh([](Checks& c) {
        auto module = DefaultLoadingManager.object(), trace = JSValue::array();
        auto fn = JSValue::function(
            [](const JSValue& self, const Args& args, const Args& a) -> JSValue {
                invoke(a[1], "push", {self.sameValue(a[0]), static_cast<double>(args.size())});
                return "dynamic";
            },
            {module, trace});
        for (const char* name :
             {"itemStart", "itemEnd", "itemError", "resolveURL", "setURLModifier", "addHandler",
              "removeHandler", "getHandler", "abort"}) {
            JSPropertyDescriptor d;
            d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue& self, const Args&, const Args& a) -> JSValue {
                    invoke(a[1], "push", {self.sameValue(a[0]), "getter"});
                    return a[2];
                },
                {module, trace, fn});
            module.defineProperty(name, d);
        }
        DefaultLoadingManager.itemStart("a");
        DefaultLoadingManager.itemEnd("a");
        DefaultLoadingManager.itemError("a");
        c.emplace_back("resolve return",
                       DefaultLoadingManager.resolveURL("a").strictEquals("dynamic"));
        c.emplace_back("setURLModifier wrapper chain",
                       &DefaultLoadingManager.setURLModifier(nullptr) == &DefaultLoadingManager);
        c.emplace_back("addHandler wrapper chain", &DefaultLoadingManager.addHandler(
                                                       nullptr, nullptr) == &DefaultLoadingManager);
        c.emplace_back("removeHandler wrapper chain",
                       &DefaultLoadingManager.removeHandler(nullptr) == &DefaultLoadingManager);
        c.emplace_back("getHandler return",
                       DefaultLoadingManager.getHandler("a").strictEquals("dynamic"));
        c.emplace_back("abort wrapper chain",
                       &DefaultLoadingManager.abort() == &DefaultLoadingManager);
        c.emplace_back("dynamic getter precedes invocation",
                       trace.toJSON()->dump() ==
                           "[true,\"getter\",true,1,true,\"getter\",true,1,true,\"getter\",true,1,"
                           "true,\"getter\",true,1,true,\"getter\",true,1,true,\"getter\",true,2,"
                           "true,\"getter\",true,1,true,\"getter\",true,1,true,\"getter\",true,0]");
    });
}
TEST_CASE("DefaultLoadingManager fields and calls reject foreign handles before resolving",
          "[loaders][default-manager][thread]") {
    auto foreign = JSValue::object();
    fresh([&](Checks& c) {
        c.emplace_back("entry empty", !JSValue::hasActiveHeap());
        c.emplace_back("field rejects", rejects([&] { DefaultLoadingManager.onLoad = foreign; }));
        c.emplace_back("start rejects", rejects([&] { DefaultLoadingManager.itemStart(foreign); }));
        c.emplace_back("end rejects", rejects([&] { DefaultLoadingManager.itemEnd(foreign); }));
        c.emplace_back("error rejects", rejects([&] { DefaultLoadingManager.itemError(foreign); }));
        c.emplace_back("resolve rejects",
                       rejects([&] { DefaultLoadingManager.resolveURL(foreign); }));
        c.emplace_back("modifier rejects",
                       rejects([&] { DefaultLoadingManager.setURLModifier(foreign); }));
        c.emplace_back("regex rejects",
                       rejects([&] { DefaultLoadingManager.addHandler(foreign, 7); }));
        c.emplace_back("loader rejects",
                       rejects([&] { DefaultLoadingManager.addHandler(7, foreign); }));
        c.emplace_back("remove rejects",
                       rejects([&] { DefaultLoadingManager.removeHandler(foreign); }));
        c.emplace_back("handler rejects",
                       rejects([&] { DefaultLoadingManager.getHandler(foreign); }));
        c.emplace_back("no rejection allocates", !JSValue::hasActiveHeap());
    });
}
TEST_CASE("Loaders retain the default manager identity across instances",
          "[loaders][default-manager]") {
    auto keeper = DefaultLoadingManager.object();
    Loader one, two;
    REQUIRE(one.manager.value().sameValue(keeper));
    REQUIRE(two.manager.value().sameValue(keeper));
    auto custom = JSValue::object();
    Loader third(custom);
    REQUIRE(third.manager.value().sameValue(custom));
}
TEST_CASE("DefaultLoadingManager real consumers release the heap after the last keeper",
          "[loaders][default-manager][gc]") {
    for (const auto* mode : {"loader", "controller", "abort"})
        fresh([&](Checks& c) {
            JSWeakValue module, controller, signal;
            c.emplace_back(std::string(mode) + " entry empty", !JSValue::hasActiveHeap());
            {
                if (std::string(mode) == "loader") {
                    Loader value;
                    module = JSWeakValue(value.manager.value());
                    auto cancellation = value.manager.get("abortController");
                    controller = JSWeakValue(cancellation);
                    signal = JSWeakValue(cancellation.get("signal"));
                } else {
                    auto keeper = DefaultLoadingManager.object();
                    module = JSWeakValue(keeper);
                    auto value = DefaultLoadingManager.abortController.value();
                    controller = JSWeakValue(value);
                    signal = JSWeakValue(value.get("signal"));
                    if (std::string(mode) == "abort")
                        DefaultLoadingManager.abort();
                }
            }
            c.emplace_back(std::string(mode) + " singleton released", module.expired());
            c.emplace_back(std::string(mode) + " controller released", controller.expired());
            c.emplace_back(std::string(mode) + " signal released", signal.expired());
            c.emplace_back(std::string(mode) + " heap released", !JSValue::hasActiveHeap());
        });
}

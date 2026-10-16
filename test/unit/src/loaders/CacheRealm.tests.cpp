#include "catch.hpp"
#include "core/JSMember.h"
#include "core/JSNumber.h"
#include "loaders/Cache.h"
#include "renderers/shaders/UniformsLib.h"
#include <exception>
#include <thread>

namespace {
using Checks = std::vector<std::pair<std::string, bool>>;
using Args = std::vector<JSValue>;
template <class F> Checks fresh(F run) {
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
    return checks;
}
void verify(const Checks& checks) {
    for (const auto& [name, passed] : checks) {
        INFO(name);
        REQUIRE(passed);
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
JSValue invoke(const JSValue& object, const char* name, const Args& args = {}) {
    return object.get(name).call(object, args);
}
void accessor(JSValue object, const char* name, const JSValue& getter, const JSValue& setter = {}) {
    JSPropertyDescriptor d;
    d.getter = getter;
    d.setter = setter;
    d.enumerable = d.configurable = true;
    object.defineProperty(name, d);
}
} // namespace

TEST_CASE("Cache facade is constant initialized without a worker heap", "[cache][realm][thread]") {
    static_assert(sizeof(JSMember) == 2 * sizeof(void*));
    verify(fresh([](Checks& c) {
        c.emplace_back("entry has no heap", !JSValue::hasActiveHeap());
        CacheStore facade;
        c.emplace_back("local facade has no heap", !JSValue::hasActiveHeap());
        c.emplace_back("enabled assignment needs keeper", rejects([] { Cache.enabled = true; }));
        c.emplace_back("nested files assignment needs keeper",
                       rejects([] { Cache.files.set("a", 7); }));
        c.emplace_back("clear needs keeper", rejects([] { Cache.clear(); }));
        c.emplace_back("remove needs keeper", rejects([] { Cache.remove("a"); }));
        c.emplace_back("add needs keeper", rejects([] { Cache.add("a", 7); }));
        c.emplace_back("no failed mutation allocates heap", !JSValue::hasActiveHeap());
        c.emplace_back("primitive lookup returns undefined", Cache.get("a").isUndefined());
        c.emplace_back("lookup leaves no hidden heap", !JSValue::hasActiveHeap());
        JSWeakValue number;
        {
            auto object = nativeNumberPrototype();
            number = JSWeakValue(object);
        }
        c.emplace_back("Number heap releases with Cache linked", number.expired());
        c.emplace_back("Number leaves no active heap", !JSValue::hasActiveHeap());
        c.emplace_back("other module guard remains active",
                       rejects([] { UniformsLib.set("cacheProof", 7); }));
        c.emplace_back("other guard leaves no heap", !JSValue::hasActiveHeap());
    }));
}
TEST_CASE("Cache has one per heap managed export and retains config only with a keeper",
          "[cache][realm][gc]") {
    verify(fresh([](Checks& c) {
        JSWeakValue module, files, payload;
        {
            auto keeper = JSValue::object();
            auto object = nativeCache();
            CacheStore facade;
            module = JSWeakValue(object);
            files = JSWeakValue(object.get("files"));
            c.emplace_back("facades share export",
                           object.sameValue(facade.object()) && object.sameValue(Cache.object()));
            c.emplace_back("initial defaults", object.get("enabled").strictEquals(false));
            const auto names = object.keys();
            c.emplace_back("literal property order",
                           names == std::vector<std::string>{"enabled", "files", "add", "get",
                                                             "remove", "clear"});
            for (const auto& name : names) {
                const auto d = object.getOwnPropertyDescriptor(name);
                c.emplace_back(name + " descriptor", d && d->writable.value_or(false) &&
                                                         d->enumerable.value_or(false) &&
                                                         d->configurable.value_or(false));
            }
            Cache.enabled = 0;
            {
                auto value = JSValue::object();
                value.set("self", value);
                payload = JSWeakValue(value);
                Cache.add("file:a", value);
            }
            object = JSValue{};
            keeper.collectGarbage();
            c.emplace_back("intrinsic retains module", !module.expired());
            c.emplace_back("module retains entry", !payload.expired());
            c.emplace_back("module retains config", Cache.enabled.strictEquals(0));
            Cache.remove("file:a");
            keeper.collectGarbage();
            c.emplace_back("removed cycle collects", payload.expired());
        }
        c.emplace_back("heap teardown releases module", module.expired());
        c.emplace_back("heap teardown releases files", files.expired());
        c.emplace_back("no root survives heap", !JSValue::hasActiveHeap());
        {
            auto next = Cache.object();
            c.emplace_back("new heap restores defaults", next.get("enabled").strictEquals(false) &&
                                                             next.get("files").keys().empty());
        }
        c.emplace_back("second heap also releases", !JSValue::hasActiveHeap());
    }));
}
TEST_CASE("Cache clear and remove preserve real dictionary reachability", "[cache][gc]") {
    verify(fresh([](Checks& c) {
        auto keeper = Cache.object();
        Cache.enabled = true;
        JSWeakValue entry, old;
        {
            auto value = JSValue::object();
            value.set("self", value);
            entry = JSWeakValue(value);
            Cache.add("a", value);
        }
        {
            auto previous = Cache.files.value();
            old = JSWeakValue(previous);
            Cache.clear();
            keeper.collectGarbage();
            c.emplace_back("clear replaces dictionary", !previous.sameValue(Cache.files.value()));
            c.emplace_back("old dictionary holds entries",
                           !entry.expired() && previous.get("a").isObject());
            c.emplace_back("new dictionary empty", Cache.get("a").isUndefined());
        }
        keeper.collectGarbage();
        c.emplace_back("released old dictionary collects", old.expired());
        c.emplace_back("released old dictionary entries collect", entry.expired());
        {
            auto value = JSValue::object();
            value.set("self", value);
            entry = JSWeakValue(value);
            Cache.add("a", value);
        }
        Cache.clear();
        keeper.collectGarbage();
        c.emplace_back("clear with no old handle collects", entry.expired());
    }));
}
TEST_CASE("Cache facade dispatches current methods and dynamic member access",
          "[cache][protocol]") {
    verify(fresh([](Checks& c) {
        auto object = Cache.object(), trace = JSValue::array();
        auto fn = JSValue::function(
            [](const JSValue& self, const Args& a, const Args& captures) -> JSValue {
                auto trace = captures[1];
                invoke(trace, "push", {self.sameValue(captures[0]), static_cast<double>(a.size())});
                if (!a.empty())
                    invoke(trace, "push", {a[0]});
                return "replacement";
            },
            {object, trace});
        for (const auto* name : {"get", "add", "remove", "clear"})
            accessor(object, name,
                     JSValue::function(
                         [](const JSValue& self, const Args&, const Args& captures) -> JSValue {
                             invoke(captures[1], "push", {self.sameValue(captures[0]), "getter"});
                             return captures[2];
                         },
                         {object, trace, fn}));
        c.emplace_back("get replacement return", Cache.get("a").strictEquals("replacement"));
        Cache.add("a", 17);
        Cache.remove("a");
        Cache.clear();
        c.emplace_back("getter before call and receiver",
                       trace.toJSON()->dump() ==
                           "[true,\"getter\",true,1,\"a\",true,\"getter\",true,2,\"a\",true,"
                           "\"getter\",true,1,\"a\",true,\"getter\",true,0]");
        auto state = JSValue::object();
        state.set("value", 3);
        accessor(object, "enabled",
                 JSValue::function([](const JSValue&, const Args&,
                                      const Args& a) -> JSValue { return a[0].get("value"); },
                                   {state}),
                 JSValue::function(
                     [](const JSValue&, const Args& args, const Args& a) -> JSValue {
                         auto s = a[0];
                         s.set("value", args[0]);
                         return {};
                     },
                     {state}));
        Cache.enabled = 19;
        c.emplace_back("field setter", state.get("value").strictEquals(19));
        c.emplace_back("field getter", Cache.enabled.strictEquals(19));
        accessor(object, "get",
                 JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
                     throw JSTypeError("getter failed");
                 }));
        c.emplace_back("getter exception propagates", rejects([] { Cache.get("a"); }));
    }));
}
TEST_CASE("Cache native functions support borrowed receivers and source metadata",
          "[cache][protocol]") {
    verify(fresh([](Checks& c) {
        auto module = Cache.object(), borrowed = JSValue::object();
        borrowed.set("enabled", 0);
        borrowed.set("files", JSValue::object());
        auto value = JSValue::object();
        module.get("add").call(borrowed, {"a", value});
        c.emplace_back("borrowed add", borrowed.get("files").get("a").sameValue(value));
        c.emplace_back("borrowed get", module.get("get").call(borrowed, {"a"}).sameValue(value));
        c.emplace_back("borrowed remove returns undefined",
                       module.get("remove").call(borrowed, {"a"}).isUndefined());
        c.emplace_back("borrowed remove deletes", borrowed.get("files").get("a").isUndefined());
        auto old = borrowed.get("files");
        module.get("clear").call(borrowed);
        c.emplace_back("borrowed clear", !old.sameValue(borrowed.get("files")));
        for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
                 {"add", 2}, {"get", 1}, {"remove", 1}, {"clear", 0}}) {
            auto fn = module.get(name);
            c.emplace_back(std::string(name) + " name", fn.get("name").strictEquals(name));
            c.emplace_back(std::string(name) + " length", fn.get("length").strictEquals(arity));
        }
    }));
}
TEST_CASE("Cache rejects foreign handles before creating a worker realm", "[cache][thread]") {
    auto foreign = JSValue::object();
    foreign.set("key", 7);
    verify(fresh([&](Checks& c) {
        c.emplace_back("entry empty", !JSValue::hasActiveHeap());
        c.emplace_back("enabled foreign", rejects([&] { Cache.enabled = foreign; }));
        c.emplace_back("files foreign", rejects([&] { Cache.files = foreign; }));
        c.emplace_back("nested foreign", rejects([&] { Cache.files.set("a", foreign); }));
        c.emplace_back("add foreign key", rejects([&] { Cache.add(foreign, 7); }));
        c.emplace_back("add foreign file", rejects([&] { Cache.add("a", foreign); }));
        c.emplace_back("get foreign key", rejects([&] { Cache.get(foreign); }));
        c.emplace_back("remove foreign key", rejects([&] { Cache.remove(foreign); }));
        c.emplace_back("equality foreign", rejects([&] { Cache.enabled.strictEquals(foreign); }));
        c.emplace_back("foreign rejection does not allocate", !JSValue::hasActiveHeap());
        {
            auto keeper = Cache.object();
            Cache.enabled = 37;
            Cache.add("key", 42);
            c.emplace_back("worker owns separate config", Cache.get("key").strictEquals(42));
        }
        c.emplace_back("worker heap ends", !JSValue::hasActiveHeap());
    }));
}

#include "animation/AnimationUtils.h"
#include "loaders/Loader.h"
#include "loaders/LoadingManager.h"
#include <iostream>
namespace {
using Args = std::vector<JSValue>;
JSValue call(const JSValue& v, const char* name, const Args& a = {}) {
    return v.get(name).call(v, a);
}
void push(const JSValue& v, const JSValue& a) { call(v, "push", {a}); }
JSValue array(const Args& a) {
    auto v = JSValue::array();
    for (const auto& x : a)
        push(v, x);
    return v;
}
JSValue encoded(const JSValue& v) { return v.isUndefined() ? JSValue("#undefined") : v; }
JSValue callbacks(const JSValue& request) {
    auto module = DefaultLoadingManager.object(), trace = JSValue::array(),
         state = JSValue::object();
    state.set("used", false);
    const auto variant = request.get("variant");
    for (const auto& [property, name] :
         std::initializer_list<std::pair<const char*, const char*>>{{"onStart", "start"},
                                                                    {"onLoad", "load"},
                                                                    {"onProgress", "progress"},
                                                                    {"onError", "error"}})
        module.set(property, JSValue::function(
                                 [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                                     Args event{c[4], self.sameValue(c[0])};
                                     for (const auto& value : a)
                                         event.push_back(value);
                                     push(c[1], array(event));
                                     auto state = c[2];
                                     if (!state.get("used").toBoolean()) {
                                         const auto mode = c[3].toNumber();
                                         const auto name = c[4].string();
                                         if ((mode == 1 && name == "start") ||
                                             (mode == 2 && name == "progress") ||
                                             (mode == 3 && name == "load")) {
                                             state.set("used", true);
                                             call(c[0], "itemStart", {"inner"});
                                         }
                                         if ((mode == 4 && name == "start") ||
                                             (mode == 5 && name == "progress") ||
                                             (mode == 6 && name == "load")) {
                                             state.set("used", true);
                                             throw JSTypeError("callback");
                                         }
                                     }
                                     return {};
                                 },
                                 {module, trace, state, variant, name}));
    const auto invoke = [&](const char* name, const JSValue& value, bool detached = false) {
        try {
            if (detached)
                module.get(name).call(JSValue::object(), {value});
            else
                call(module, name, {value});
        } catch (const JSTypeError&) {
            push(trace, "TypeError");
        }
    };
    invoke("itemStart", "a");
    invoke("itemStart", "b");
    invoke("itemEnd", "a");
    invoke("itemError", "b");
    invoke("itemEnd", "b");
    if (variant.toNumber() > 0 && variant.toNumber() < 4)
        invoke("itemEnd", "inner");
    invoke("itemStart", "next");
    invoke("itemEnd", "next");
    invoke("itemStart", "detached", true);
    invoke("itemEnd", "detached", true);
    for (const auto* field : {"onStart", "onLoad", "onProgress", "onError"})
        module.set(field, JSValue{});
    return trace;
}
JSValue handler() {
    auto trace = JSValue::array(), first = JSValue::object(), second = JSValue::object();
    first.set("global", true);
    first.set("lastIndex", 17);
    second.set("global", false);
    auto fn = JSValue::function(
        [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
            push(c[0], array({c[1], encoded(self.get("lastIndex")), a[0]}));
            if (c[1].strictEquals("first")) {
                auto o = self;
                o.set("lastIndex", 19);
                return a[0].strictEquals("first");
            }
            return true;
        },
        {trace, "first"});
    first.set("test", fn);
    second.set("test", JSValue::function(
                           [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                               push(c[0], array({"second", encoded(self.get("lastIndex")), a[0]}));
                               return true;
                           },
                           {trace}));
    DefaultLoadingManager.addHandler(first, "one").addHandler(second, "two");
    auto result =
        array({DefaultLoadingManager.getHandler("first"), DefaultLoadingManager.getHandler("first"),
               DefaultLoadingManager.getHandler("other")});
    DefaultLoadingManager.removeHandler(first);
    push(result, DefaultLoadingManager.getHandler("first"));
    DefaultLoadingManager.removeHandler(second);
    push(result, DefaultLoadingManager.getHandler("none"));
    return array({result, trace});
}
JSValue abort() {
    auto module = DefaultLoadingManager.object();
    module.set("_abortController", nullptr);
    auto first = DefaultLoadingManager.abortController.value(), signal = first.get("signal"),
         trace = JSValue::array();
    call(signal, "addEventListener",
         {"abort", JSValue::function(
                       [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                           push(c[0], array({self.sameValue(c[1]), a[0].get("type"),
                                             self.get("aborted"), self.get("reason").get("name")}));
                           return {};
                       },
                       {trace, signal})});
    const auto same = DefaultLoadingManager.abortController.value().sameValue(first);
    DefaultLoadingManager.abort();
    const auto cleared = DefaultLoadingManager._abortController.isNull();
    auto second = DefaultLoadingManager.abortController.value();
    call(first, "abort", {"ignored"});
    return array({same, cleared, !first.sameValue(second), second.get("signal").get("aborted"),
                  trace, signal.get("reason").get("name")});
}
JSValue run(const JSValue& r) {
    const auto mode = r.get("mode").string();
    if (mode == "callbacks")
        return callbacks(r);
    if (mode == "handler")
        return handler();
    if (mode == "abort")
        return abort();
    if (mode == "loader") {
        Loader one, two;
        auto custom = JSValue::object();
        Loader third(custom);
        return array({one.manager.value().sameValue(DefaultLoadingManager.object()),
                      two.manager.value().sameValue(one.manager.value()),
                      third.manager.value().sameValue(custom), one.crossOrigin.value(),
                      one.withCredentials.value(), one.path.value(), one.resourcePath.value()});
    }
    if (mode == "modifier") {
        auto trace = JSValue::array();
        DefaultLoadingManager.setURLModifier(JSValue::function(
            [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                push(c[0], array({self.isUndefined(), a[0]}));
                return a[0];
            },
            {trace}));
        const auto value = DefaultLoadingManager.resolveURL(r.get("url"));
        DefaultLoadingManager.setURLModifier(JSValue{});
        return array({value, trace});
    }
    if (mode == "metadata") {
        auto m = DefaultLoadingManager.object(), out = JSValue::array();
        for (const auto* name :
             {"itemStart", "itemEnd", "itemError", "resolveURL", "setURLModifier", "addHandler",
              "removeHandler", "getHandler", "abort"}) {
            const auto f = m.get(name);
            const auto d = m.getOwnPropertyDescriptor(name);
            push(out, array({f.get("name"), f.get("length"), d->writable.value(),
                             d->enumerable.value(), d->configurable.value()}));
        }
        auto proto = LoadingManager::prototype();
        const auto descriptor = proto.getOwnPropertyDescriptor("abortController");
        push(out, array({proto.get("constructor").get("name"),
                         proto.get("constructor").get("length"), descriptor->getter->get("name"),
                         descriptor->getter->get("length"), descriptor->enumerable.value_or(false),
                         descriptor->configurable.value_or(false)}));
        return out;
    }
    throw std::runtime_error("unknown");
}
} // namespace
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "gc") {
        auto module = DefaultLoadingManager.object(), out = JSValue::array();
        JSWeakValue weak;
        {
            auto payload = JSValue::object();
            payload.set("self", payload);
            weak = JSWeakValue(payload);
            DefaultLoadingManager.onLoad = JSValue::function(
                [](const JSValue&, const Args&, const Args& a) -> JSValue { return a[0]; },
                {payload});
        }
        module.collectGarbage();
        push(out, !weak.expired());
        DefaultLoadingManager.onLoad = JSValue{};
        module.collectGarbage();
        push(out, !weak.expired());
        auto regex = JSValue::object();
        regex.set("global", false);
        regex.set("test", JSValue::function([](const JSValue&, const Args&,
                                               const Args&) -> JSValue { return true; }));
        {
            auto payload = JSValue::object();
            payload.set("self", payload);
            weak = JSWeakValue(payload);
            DefaultLoadingManager.addHandler(regex, payload);
        }
        module.collectGarbage();
        push(out, !weak.expired());
        DefaultLoadingManager.removeHandler(regex);
        module.collectGarbage();
        push(out, !weak.expired());
        std::cout << out.toJSON()->dump();
        return 0;
    }
    const std::string input(std::istreambuf_iterator<char>(std::cin), {});
    auto requests = Animation_detail::parseJSON(input), out = JSValue::array();
    for (std::uint32_t i = 0; i < requests.length(); ++i)
        try {
            push(out, run(requests.get(std::to_string(i))));
        } catch (const JSTypeError&) {
            push(out, "TypeError");
        }
    std::cout << out.toJSON()->dump();
}

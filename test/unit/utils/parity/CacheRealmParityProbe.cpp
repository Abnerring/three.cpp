#include "animation/AnimationUtils.h"
#include "loaders/Cache.h"
#include <iostream>
namespace {
using Args = std::vector<JSValue>;
JSValue call(const JSValue& o, const char* name, const Args& args = {}) {
    return o.get(name).call(o, args);
}
void event(const JSValue& state, const char* name) {
    call(state.get("trace"), "push", {name});
    if (state.get("throw").strictEquals(name))
        throw JSTypeError(name);
}
void accessor(JSValue o, const char* name, const JSValue& get, const JSValue& set = {}) {
    JSPropertyDescriptor d;
    d.getter = get;
    d.setter = set;
    d.enumerable = d.configurable = true;
    o.defineProperty(name, d);
}
JSValue encode(const JSValue& value) {
    if (value.isUndefined())
        return "#undefined";
    if (value.isCallable())
        return "#function";
    return value;
}
JSValue run(const JSValue& request) {
    auto module = Cache.object();
    auto state = JSValue::object();
    state.set("trace", JSValue::array());
    state.set("throw", request.get("throw"));
    state.set("enabled", request.get("enabled"));
    state.set("stored", 12);
    auto dictionary = JSValue::object();
    state.set("files", dictionary);
    auto target = JSValue::object();
    accessor(target, "enabled",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     event(c[0], "enabled");
                     return c[0].get("enabled");
                 },
                 {state}));
    accessor(target, "files",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     event(c[0], "files");
                     return c[0].get("files");
                 },
                 {state}),
             JSValue::function(
                 [](const JSValue&, const Args& a, const Args& c) -> JSValue {
                     event(c[0], "setFiles");
                     auto s = c[0];
                     s.set("files", a[0]);
                     return {};
                 },
                 {state}));
    accessor(dictionary, "slot",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) -> JSValue {
                     event(c[0], "dictGet");
                     return c[0].get("stored");
                 },
                 {state}),
             JSValue::function(
                 [](const JSValue&, const Args& a, const Args& c) -> JSValue {
                     event(c[0], "dictSet");
                     auto s = c[0];
                     s.set("stored", a[0]);
                     return {};
                 },
                 {state}));
    auto key = JSValue::object();
    for (const auto* name : {"slice", "indexOf", "toString"})
        accessor(key, name,
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args& c) -> JSValue {
                         const auto name = c[1].string();
                         event(c[0], (name + "Get").c_str());
                         return JSValue::function(
                             [](const JSValue&, const Args& a, const Args& c) -> JSValue {
                                 const auto name = c[1].string();
                                 event(c[0], (name + "Call").c_str());
                                 if (name == "slice") {
                                     call(c[0].get("trace"), "push", {encode(a[0])});
                                     return c[2];
                                 }
                                 if (name == "indexOf")
                                     return c[2];
                                 return "slot";
                             },
                             {c[0], c[1], c[2]});
                     },
                     {state, name,
                      std::string(name) == "slice" ? request.get("url") : request.get("index")}));
    JSValue result;
    try {
        const auto op = request.get("op").string();
        result = module.get(op).call(target, {key, 39});
    } catch (const JSTypeError&) {
        result = "TypeError";
    }
    auto output = JSValue::array();
    for (const auto& value :
         std::initializer_list<JSValue>{encode(result), state.get("trace"), state.get("stored"),
                                        state.get("files").sameValue(dictionary),
                                        dictionary.getOwnPropertyDescriptor("slot").has_value()})
        call(output, "push", {value});
    return output;
}
} // namespace
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "gc") {
        auto module = Cache.object();
        Cache.enabled = true;
        JSWeakValue payload;
        auto output = JSValue::array();
        {
            auto v = JSValue::object();
            v.set("self", v);
            payload = JSWeakValue(v);
            Cache.add("a", v);
        }
        module.collectGarbage();
        call(output, "push", {!payload.expired()});
        Cache.remove("a");
        module.collectGarbage();
        call(output, "push", {!payload.expired()});
        {
            auto v = JSValue::object();
            v.set("self", v);
            payload = JSWeakValue(v);
            Cache.add("a", v);
        }
        {
            auto old = Cache.files.value();
            Cache.clear();
            module.collectGarbage();
            call(output, "push", {!payload.expired()});
        }
        module.collectGarbage();
        call(output, "push", {!payload.expired()});
        {
            auto v = JSValue::object();
            v.set("self", v);
            payload = JSWeakValue(v);
            Cache.add("a", v);
        }
        Cache.clear();
        module.collectGarbage();
        call(output, "push", {!payload.expired()});
        std::cout << output.toJSON()->dump();
        return 0;
    }
    const std::string input(std::istreambuf_iterator<char>(std::cin), {});
    auto requests = Animation_detail::parseJSON(input), out = JSValue::array();
    for (std::uint32_t i = 0; i < requests.length(); ++i)
        call(out, "push", {run(requests.get(std::to_string(i)))});
    std::cout << out.toJSON()->dump();
}

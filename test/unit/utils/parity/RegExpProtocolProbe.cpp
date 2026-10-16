#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include <iostream>
namespace {
using Args = std::vector<JSValue>;
JSPropertyDescriptor data(JSValue value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = true;
    d.configurable = true;
    d.enumerable = true;
    return d;
}
void event(const JSValue& state, const std::string& label) {
    auto events = state.get("events");
    events.set(std::to_string(events.length()), label);
    state.collectGarbage();
    if (events.length() == state.get("fail").number())
        throw JSTypeError("sentinel");
}
void getter(const JSValue& state, JSValue target, const JSValue& key, const std::string& label,
            const JSValue& value) {
    JSPropertyDescriptor d;
    d.configurable = true;
    d.getter = JSValue::function(
        +[](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], c[1].string());
            return c[2];
        },
        {state, label, value});
    target.defineProperty(key, d);
}
JSValue convert(const JSValue& state, const std::string& label, const JSValue& value) {
    auto out = JSValue::object();
    out.setPrimitiveConverter(JSValue::function(
        +[](const JSValue&, const Args& args, const Args& c) -> JSValue {
            event(c[0], c[1].string() + ":" + args[0].string());
            return c[2];
        },
        {state, label, value}));
    return out;
}
JSValue lastIndex(const JSValue& state, double initial) {
    auto target = JSValue::object();
    auto cell = JSValue::object();
    cell.set("value", initial);
    JSPropertyDescriptor d;
    d.configurable = true;
    d.getter = JSValue::function(
        +[](const JSValue&, const Args&, const Args& c) -> JSValue {
            event(c[0], "last.get");
            return c[1].get("value");
        },
        {state, cell});
    d.setter = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& c) -> JSValue {
            event(c[0], "last.set:" + args[0].toString());
            auto cell = c[1];
            cell.set("value", args[0]);
            return {};
        },
        {state, cell});
    target.defineProperty("lastIndex", d);
    return target;
}
JSValue tagged(const JSValue& value) {
    if (value.isUndefined())
        return "<undefined>";
    if (value.type() == JSValue::Type::Array) {
        auto result = JSValue::array();
        for (unsigned i = 0; i < value.length(); ++i)
            result.set(std::to_string(i), tagged(value.get(std::to_string(i))));
        return result;
    }
    return value;
}
JSValue run(int mode, int variant, const JSValue& state) {
    const auto sp = nativeStringPrototype(), rp = nativeRegExpPrototype();
    if (mode == 0 || mode == 1) {
        auto self = convert(state, "self", "aba");
        auto search = convert(state, "search", "a");
        auto replacement = convert(state, "replacement", "$&x");
        auto method = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                event(c[0], "replace.call");
                return self.sameValue(c[1]) && args[0].sameValue(c[2]) && args[1].sameValue(c[3]);
            },
            {state, search, self, replacement});
        getter(state, search, JSValue::wellKnownSymbol("replace"), "replace.get",
               mode == 0 ? method : JSValue{});
        return sp.get("replace").call(self, {search, replacement});
    }
    if (mode == 2 || mode == 3) {
        auto self = convert(state, "self", "a:b");
        auto separator = convert(state, "separator", ":");
        auto limit = convert(state, "limit", variant ? 0 : 4);
        auto method = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                event(c[0], "split.call");
                return self.sameValue(c[1]) && args[0].sameValue(c[2]) && args[1].sameValue(c[3]);
            },
            {state, separator, self, limit});
        getter(state, separator, JSValue::wellKnownSymbol("split"), "split.get",
               mode == 2 ? method : JSValue{});
        return sp.get("split").call(self, {separator, limit});
    }
    if (mode == 4) {
        auto regexp = createNativeRegExp("a", variant ? "g" : "");
        regexp.set("lastIndex", convert(state, "last", 1));
        auto text = convert(state, "input", "aba");
        const auto result = rp.get("exec").call(regexp, {text});
        auto out = JSValue::object();
        out.set("match", result);
        out.set("index", result.isNull() ? JSValue{} : result.get("index"));
        out.set("lastIndex", variant ? regexp.get("lastIndex") : JSValue("object"));
        return out;
    }
    if (mode == 5 || mode == 6) {
        auto receiver = lastIndex(state, 99);
        getter(state, receiver, "flags", "flags.get",
               convert(state, "flags", variant ? "gu" : "g"));
        auto cursor = JSValue::object();
        cursor.set("index", 0);
        auto method = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                event(c[0], "exec.call");
                auto cursor = c[1];
                const int index = int(cursor.get("index").number());
                cursor.set("index", index + 1);
                if (index == 2)
                    return nullptr;
                auto result = JSValue::object();
                const auto prefix = std::string("result") + std::to_string(index) + ".";
                getter(c[0], result, "length", prefix + "length", 2);
                getter(c[0], result, "0", prefix + "0",
                       convert(c[0], prefix + "match", index ? "" : "a"));
                getter(c[0], result, "index", prefix + "index",
                       convert(c[0], prefix + "position", index ? 2 : 1));
                getter(c[0], result, "1", prefix + "1",
                       index ? JSValue{} : convert(c[0], prefix + "capture", "C"));
                auto groups = JSValue::object();
                getter(c[0], groups, "x", prefix + "named",
                       convert(c[0], prefix + "namedValue", "N"));
                getter(c[0], result, "groups", prefix + "groups",
                       c[2].toBoolean() ? groups : JSValue{});
                return result;
            },
            {state, cursor, mode == 6});
        getter(state, receiver, "exec", "exec.get", method);
        auto callback = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                event(c[0], "callback:" + args[0].string() + ":" + args[2].toString() + ":" +
                                (self.isUndefined() ? "undefined" : "other"));
                return convert(c[0], "callback.result", "<" + args[0].string() + ">");
            },
            {state});
        auto replacement = mode == 5 ? callback : convert(state, "replacement", "$<x>:$1:$<x>:$&");
        return rp.get(JSValue::wellKnownSymbol("replace"))
            .call(receiver, {convert(state, "input", "abc"), replacement});
    }
    if (mode == 7) {
        auto receiver = JSValue::object();
        auto splitter = lastIndex(state, 0);
        auto method = JSValue::function(
            +[](const JSValue& self, const Args&, const Args& c) -> JSValue {
                event(c[0], "exec.call");
                if (self.get("lastIndex").number() != 1)
                    return nullptr;
                auto target = self;
                target.set("lastIndex", 2);
                auto result = JSValue::object();
                getter(c[0], result, "length", "result.length", 2);
                getter(c[0], result, "1", "result.1", "D");
                return result;
            },
            {state});
        getter(state, splitter, "exec", "exec.get", method);
        auto ctor = JSValue::object();
        auto species = JSValue::callableBuiltinConstructor(
            +[](const JSValue& self, const JSValue& target, const Args& args,
                const Args& c) -> JSValue {
                event(c[0], "species.construct:" + args[1].string() + ":" +
                                (self.isUndefined() && !target.isUndefined() ? "new" : "call"));
                return c[1];
            },
            {state, splitter});
        getter(state, ctor, JSValue::wellKnownSymbol("species"), "species.get", species);
        getter(state, receiver, "constructor", "constructor.get", ctor);
        getter(state, receiver, "flags", "flags.get", convert(state, "flags", variant ? "y" : ""));
        return rp.get(JSValue::wellKnownSymbol("split"))
            .call(receiver,
                  {convert(state, "input", "a:b"), convert(state, "limit", variant ? 0 : 5)});
    }
    if (mode == 8) {
        auto pattern = JSValue::object();
        getter(state, pattern, JSValue::wellKnownSymbol("match"), "match.get", true);
        getter(state, pattern, "source", "source.get", convert(state, "source", "a"));
        getter(state, pattern, "flags", "flags.get", convert(state, "flags", "g"));
        getter(state, pattern, "constructor", "constructor.get",
               variant ? nativeRegExpConstructor() : JSValue{});
        auto result = nativeRegExpConstructor().call({}, {pattern});
        if (result.sameValue(pattern))
            return "same";
        return result.get("toString").call(result);
    }
    if (mode == 9) {
        auto receiver = JSValue::object();
        const char* names[] = {"hasIndices", "global",  "ignoreCase",  "multiline",
                               "dotAll",     "unicode", "unicodeSets", "sticky"};
        for (unsigned i = 0; i < 8; ++i)
            getter(state, receiver, names[i], names[i],
                   variant ? JSValue::object() : JSValue(i % 2 == 0));
        return rp.getOwnPropertyDescriptor("flags")->getter->call(receiver);
    }
    if (mode == 10) {
        auto receiver = JSValue::object();
        getter(state, receiver, "source", "source.get", convert(state, "source", "a"));
        getter(state, receiver, "flags", "flags.get", convert(state, "flags", "g"));
        return rp.get("toString").call(receiver);
    }
    if (mode == 11) {
        auto receiver = JSValue::object();
        auto fn = JSValue::function(
            +[](const JSValue&, const Args& args, const Args& c) -> JSValue {
                event(c[0], "exec.call:" + args[0].string());
                return c[1].toBoolean() ? JSValue(3) : JSValue(nullptr);
            },
            {state, variant});
        getter(state, receiver, "exec", "exec.get", fn);
        return rp.get("test").call(receiver, {convert(state, "input", "abc")});
    }
    if (mode == 12) {
        auto regexp = createNativeRegExp("a");
        regexp.defineProperty(JSValue::wellKnownSymbol("match"),
                              data(variant ? JSValue(false) : JSValue{}));
        return sp.get("startsWith").call("/a/", {regexp});
    }
    if (mode == 13) {
        auto self = convert(state, "input", "  Straße  ");
        return sp.get(variant ? "toUpperCase" : "trim").call(self);
    }
    if (mode == 14) {
        auto out = JSValue::array();
        unsigned index = 0;
        auto append = [&](const std::string& name, const JSValue& owner, const JSValue& key) {
            auto d = owner.getOwnPropertyDescriptor(key);
            auto fn = d->value ? *d->value : *d->getter;
            auto item = JSValue::object();
            item.set("key", name);
            item.set("name", fn.get("name"));
            item.set("length", fn.get("length"));
            item.set("constructible", fn.isConstructible());
            item.set("prototype", fn.hasOwn("prototype"));
            item.set("enumerable", *d->enumerable);
            item.set("configurable", *d->configurable);
            item.set("writable", d->writable ? JSValue(*d->writable) : JSValue("accessor"));
            out.set(std::to_string(index++), item);
        };
        for (const auto* name :
             {"exec", "test", "toString", "source", "flags", "global", "sticky", "unicodeSets"})
            append(std::string("RegExp.") + name, rp, name);
        for (const auto* name : {"replace", "split", "match"})
            append(std::string("RegExp.Symbol.") + name, rp, JSValue::wellKnownSymbol(name));
        for (const auto* name : {"replace", "split", "trim", "toUpperCase"})
            append(std::string("String.") + name, sp, name);
        append("RegExp.constructor", rp, "constructor");
        auto names = JSValue::array();
        unsigned n = 0;
        for (const auto& name : rp.ownPropertyNames())
            names.set(std::to_string(n++), name);
        auto result = JSValue::object();
        result.set("methods", out);
        result.set("names", names);
        result.set("prototypeWritable",
                   *nativeRegExpConstructor().getOwnPropertyDescriptor("prototype")->writable);
        return result;
    }
    return {};
}
} // namespace
int main() {
    auto realm = JSValue::object();
    int mode, variant, fail;
    while (std::cin >> mode >> variant >> fail) {
        auto state = JSValue::object();
        state.set("events", JSValue::array());
        state.set("fail", fail);
        auto out = JSValue::object();
        try {
            out.set("value", tagged(run(mode, variant, state)));
        } catch (const JSTypeError&) {
            out.set("error", "TypeError");
        } catch (const JSRegExpUnsupported&) {
            out.set("error", "Unsupported");
        } catch (const std::exception& e) {
            out.set("nativeError", e.what());
        }
        out.set("events", state.get("events"));
        std::cout << out.toJSON()->dump() << '\n';
        realm.collectGarbage();
    }
}

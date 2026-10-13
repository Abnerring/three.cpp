#include "core/JSNumber.h"
#include "core/JSString.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
namespace {
using Args = std::vector<JSValue>;
void trace(const JSValue& log, const std::string& s) {
    auto copy = log;
    copy.set(std::to_string(copy.length()), s);
}
void literal(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
void getter(JSValue object, const JSValue& key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.getter = value;
    d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
JSValue receiver(unsigned kind, const JSValue& log) {
    switch (kind) {
    case 0:
        return 1.25;
    case 1:
        return -0.0;
    case 2:
        return NAN;
    case 3:
        return INFINITY;
    case 4:
        return -INFINITY;
    case 5: {
        auto o = createNativeNumberObject(1.25);
        literal(o, "valueOf",
                JSValue::function(
                    +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                        trace(c[0], "receiver-valueOf");
                        throw JSTypeError("unexpected");
                    },
                    {log}));
        return o;
    }
    case 6:
        return nativeNumberPrototype();
    case 7: {
        auto o = JSValue::object();
        literal(o, "valueOf",
                JSValue::function(
                    +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                        trace(c[0], "receiver-valueOf");
                        return 1.25;
                    },
                    {log}));
        return o;
    }
    case 8:
        return JSValue::object(nativeNumberPrototype());
    case 9:
        return true;
    case 10:
        return "1.25";
    case 11:
        return nullptr;
    case 12:
        return {};
    case 13:
        return JSValue::symbol("receiver");
    case 14:
        return JSValue::array();
    case 15:
        return JSValue::function(
            +[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    default:
        return createNativeStringObject("1.25");
    }
}
JSValue action(const JSValue& self, const Args& args, const Args& c) {
    const auto kind = static_cast<unsigned>(c[0].number());
    const auto log = c[1];
    if (kind == 11) {
        trace(log, "primitive:" + args[0].string());
        return 3;
    }
    if (kind == 13) {
        trace(log, "valueOf");
        return JSValue::object();
    }
    if (kind == 14) {
        trace(log, "valueOf");
        throw JSRangeError("fraction");
    }
    if (kind == 15) {
        trace(log, "primitive:" + args[0].string());
        throw JSTypeError("fraction");
    }
    if (kind == 16) {
        trace(log, "valueOf");
        const JSValue n = 1.25;
        trace(log, "inner:" + n.get("toFixed").call(n, {1}).string());
        return 2;
    }
    if (kind == 17) {
        trace(log, "valueOf");
        nativeNumberPrototype().set(
            "toFixed", JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue {
                return "replaced";
            }));
        return 2;
    }
    if (kind == 19) {
        trace(log, "valueOf");
        return JSValue::symbol("fraction");
    }
    if (kind == 20) {
        trace(log, "primitive:" + args[0].string());
        return JSValue::object();
    }
    if (kind == 23) {
        trace(log, "toString");
        return "4";
    }
    trace(log, "valueOf");
    return 2;
}
JSValue input(unsigned kind, const JSValue& log) {
    switch (kind) {
    case 0:
        return {};
    case 1:
        return nullptr;
    case 2:
        return false;
    case 3:
        return true;
    case 4:
        return "2";
    case 5:
        return "100.9";
    case 6:
        return "-0.9";
    case 7:
        return "-1";
    case 8:
        return "101";
    case 9:
        return JSValue::symbol("fraction");
    case 18:
        return createNativeNumberObject(2);
    case 22:
        return 10;
    case 24:
        return NAN;
    case 25:
        return INFINITY;
    case 26:
        return -INFINITY;
    }
    auto o = JSValue::object();
    const auto fn = JSValue::function(action, {kind, log});
    if (kind == 11 || kind == 15 || kind == 20)
        getter(o, JSValue::wellKnownSymbol("toPrimitive"),
               JSValue::function(
                   +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                       trace(c[0], "get-primitive");
                       return c[1];
                   },
                   {log, fn}));
    else if (kind == 12)
        literal(o, "valueOf", 2),
            getter(o, JSValue::wellKnownSymbol("toPrimitive"),
                   JSValue::function(
                       +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                           trace(c[0], "get-primitive");
                           return 1;
                       },
                       {log}));
    else if (kind == 21) {
        getter(o, "valueOf",
               JSValue::function(
                   +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                       trace(c[0], "get-valueOf");
                       return 0;
                   },
                   {log}));
        getter(o, "toString",
               JSValue::function(
                   +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                       trace(c[0], "get-toString");
                       return c[1];
                   },
                   {log, JSValue::function(action, {23, log})}));
    } else {
        literal(o, "valueOf", fn);
        if (kind == 13)
            literal(o, "toString", JSValue::function(action, {23, log}));
    }
    return o;
}
std::string encoded(const JSValue& value) {
    if (value.type() == JSValue::Type::Number)
        return "number:" + (value.number() == 0 && std::signbit(value.number()) ? std::string("-0")
                                                                                : value.toString());
    return "string:" + value.string();
}
} // namespace
int main() {
    const auto realm = JSValue::object();
    JsonValue::Array results;
    for (const auto* method : {"toFixed", "valueOf", "toString"})
        for (unsigned r = 0; r < 17; ++r)
            for (unsigned f = 0; f < 27; ++f) {
                auto prototype = nativeNumberPrototype();
                const auto original = *prototype.getOwnPropertyDescriptor("toFixed");
                const auto fn = prototype.get(method);
                const auto log = JSValue::array();
                const auto self = receiver(r, log), fraction = input(f, log);
                JsonValue::Object result{{"method", method}, {"receiver", r}, {"fraction", f}};
                try {
                    result["value"] = encoded(fn.call(self, {fraction}));
                } catch (const JSRangeError&) {
                    result["error"] = "RangeError";
                } catch (const JSTypeError&) {
                    result["error"] = "TypeError";
                } catch (const JSNumberRadixUnsupported&) {
                    result["boundary"] = "non-decimal radix";
                }
                prototype.defineProperty("toFixed", original);
                result["trace"] = *log.toJSON();
                results.emplace_back(result);
            }

    {
        auto prototype = nativeNumberPrototype();
        auto object = JSValue::object();
        JsonValue::Array methods;
        for (const auto* name : {"toFixed", "toString", "valueOf"}) {
            const auto d = *prototype.getOwnPropertyDescriptor(name);
            const auto fn = *d.value;
            bool constructible = true;
            try {
                fn.construct();
            } catch (const JSTypeError&) {
                constructible = false;
            }
            methods.emplace_back(
                JsonValue::Object{{"name", fn.get("name").string()},
                                  {"length", fn.get("length").number()},
                                  {"writable", d.writable.value()},
                                  {"enumerable", d.enumerable.value()},
                                  {"configurable", d.configurable.value()},
                                  {"constructible", constructible},
                                  {"ownPrototype", fn.hasOwn("prototype")},
                                  {"source", fn.get("toString").call(fn).string()}});
        }
        auto boxed = createNativeNumberObject(-0.0);
        JsonValue::Array boxedNames;
        for (const auto& key : boxed.ownPropertyKeys())
            boxedNames.emplace_back(key.toString());
        results.emplace_back(
            JsonValue::Object{{"metadata", methods},
                              {"prototypeValue", encoded(prototype.get("valueOf").call(prototype))},
                              {"prototypeBrand", object.get("toString").call(prototype).string()},
                              {"boxedBrand", object.get("toString").call(boxed).string()},
                              {"boxedValue", encoded(boxed.get("valueOf").call(boxed))},
                              {"boxedOwnNames", boxedNames}});
    }
    {
        auto prototype = nativeNumberPrototype();
        const auto original = *prototype.getOwnPropertyDescriptor("toFixed");
        const auto log = JSValue::array();
        const auto symbol = JSValue::symbol("dynamic");
        const auto read = JSValue::function(
            +[](const JSValue& self, const Args&, const Args& c) -> JSValue {
                trace(c[0], "get:" + encoded(self));
                return c[1];
            },
            {log, *original.value});
        getter(prototype, "toFixed", read);
        getter(prototype, symbol, read);
        const JSValue number = 1.25;
        const auto fixed = number.get("toFixed");
        trace(log, "result:" + fixed.call(number, {1}).string());
        const auto symbolMethod = number.get(symbol);
        trace(log, "symbol:" + symbolMethod.call(number, {2}).string());
        const auto other = number.get("toFixed", JSValue(-2.5));
        trace(log, "other:" + other.call(-2.5, {0}).string());
        prototype.defineProperty("toFixed", original);
        prototype.erase(symbol);
        results.emplace_back(JsonValue::Object{{"mutation", *log.toJSON()}});
    }
    std::cout << JsonValue(std::move(results)).dump();
}

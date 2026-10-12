#include "core/JSValue.h"
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {
using Args = std::vector<JSValue>;
using Descriptor = JSPropertyDescriptor;
Descriptor data(const JSValue& value, bool writable = true, bool enumerable = true, bool configurable = true) {
    Descriptor result;
    result.value = value; result.writable = writable;
    result.enumerable = enumerable; result.configurable = configurable;
    return result;
}
JsonValue names(const std::vector<std::string>& keys) {
    JsonValue::Array result;
    for (const auto& key : keys) result.emplace_back(key);
    return result;
}
template<class F> JsonValue outcome(F operation) {
    try { return operation(); }
    catch (const JSTypeError&) { return JsonValue::Object{{"error", "TypeError"}}; }
    catch (const JSRangeError&) { return JsonValue::Object{{"error", "RangeError"}}; }
}
JSValue readStored(const JSValue& receiver, const Args&, const Args&) { return receiver.get("stored"); }
JSValue writeStored(const JSValue& receiver, const Args& args, const Args&) {
    auto value = receiver; value.set("stored", args.at(0)); return {};
}
JSValue firstCapture(const JSValue&, const Args&, const Args& captures) { return captures.at(0); }
void log(const JSValue& value, const std::string& message) {
    auto array = value; array.set(std::to_string(array.length()), message);
}
JsonValue scenarios() {
    JsonValue result = JsonValue::Object{};
    {
        auto payload = JSValue::object(); payload.set("answer", 42);
        auto fn = JSValue::function(firstCapture, {payload}, "fn", 1), alias = fn;
        result["function"] = JsonValue::Object{{"identity", fn.strictEquals(alias)},
            {"different", fn.strictEquals(JSValue::function(firstCapture, {payload}))},
            {"captured", fn.call().strictEquals(payload)}, {"names", names(fn.ownPropertyNames())},
            {"keys", names(fn.keys())}, {"name", fn.get("name").string()},
            {"length", fn.get("length").number()}, {"callNonfunction", outcome([] { return JSValue(1).call().trace(); })}};
    }
    {
        auto prototype = JSValue::object(), child = JSValue::object(prototype), other = JSValue::object();
        prototype.set("stored", 1); child.set("stored", 10); other.set("stored", 20);
        Descriptor accessor;
        accessor.getter = JSValue::function(readStored); accessor.setter = JSValue::function(writeStored);
        accessor.enumerable = true; accessor.configurable = true;
        prototype.defineProperty("value", accessor);
        JsonValue output = JsonValue::Object{{"inherited", child.has("value")}, {"own", child.hasOwn("value")},
            {"get", child.get("value").number()}, {"receiverGet", prototype.get("value", other).number()}};
        child.set("value", 11); prototype.reflectSet("value", 21, other);
        output["child"] = child.get("stored").number(); output["other"] = other.get("stored").number();
        output["prototype"] = prototype.get("stored").number(); output["keys"] = names(child.keys());
        output["cycle"] = prototype.setPrototype(child);
        child.preventExtensions(); output["preventedPrototype"] = child.setPrototype(nullptr);
        output["samePrototype"] = child.setPrototype(prototype);
        output["addPrevented"] = child.reflectSet("new", 3, child);
        output["isExtensible"] = child.isExtensible();
        result["prototype"] = output;
    }
    {
        auto object = JSValue::object();
        Descriptor frozen; frozen.value = -0.0; object.defineProperty("zero", frozen);
        frozen.value = std::numeric_limits<double>::quiet_NaN(); object.defineProperty("nan", frozen);
        Descriptor update; update.value = 0.0;
        JsonValue output = JsonValue::Object{{"positiveZero", object.defineOwnProperty("zero", update)}};
        update.value = -0.0; output["negativeZero"] = object.defineOwnProperty("zero", update);
        update.value = std::numeric_limits<double>::quiet_NaN(); output["NaN"] = object.defineOwnProperty("nan", update);
        output["erase"] = object.erase("zero"); output["names"] = names(object.ownPropertyNames());
        output["keys"] = names(object.keys());
        auto child = JSValue::object(object);
        output["inheritedReadonly"] = child.reflectSet("zero", 2, child);
        output["ownAfterFailedSet"] = child.hasOwn("zero");
        output["strictSet"] = outcome([&] { child.set("zero", 2); return JsonValue(true); });
        Descriptor invalid; invalid.getter = nullptr;
        output["badGetter"] = outcome([&] { return JsonValue(object.defineOwnProperty("bad", invalid)); });
        invalid.getter = JSValue::function(readStored); invalid.value = 2;
        output["mixedDescriptor"] = outcome([&] { return JsonValue(object.defineOwnProperty("bad", invalid)); });
        Descriptor undefinedGetter; undefinedGetter.getter = JSValue{}; undefinedGetter.enumerable = true;
        object.defineProperty("missingGetter", undefinedGetter);
        output["missingGetter"] = object.get("missingGetter").trace();
        output["missingSetter"] = object.reflectSet("missingGetter", 2, object);
        result["descriptors"] = output;
    }
    {
        auto array = JSValue::array(); array.set("0", 0); array.set("2", 2); array.set("4", 4);
        Descriptor fixed; fixed.configurable = false; array.defineProperty("2", fixed);
        Descriptor shrink; shrink.value = 1; shrink.writable = false;
        JsonValue output = JsonValue::Object{{"shrink", array.defineOwnProperty("length", shrink)},
            {"length", array.length()}, {"keys", names(array.keys())}, {"names", names(array.ownPropertyNames())},
            {"lengthWritable", *array.getOwnPropertyDescriptor("length")->writable},
            {"extend", array.reflectSet("3", 3, array)}};
        auto numeric = JSValue::array(4), counter = JSValue::object(), length = JSValue::object(); counter.set("calls", 0);
        length.set("valueOf", JSValue::function(+[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            auto count = captures.at(0); count.set("calls", count.get("calls").toNumber() + 1); return 2;
        }, {counter}));
        numeric.set("length", length); output["coercedLength"] = numeric.length(); output["calls"] = counter.get("calls").number();
        numeric.set("length", "3"); output["stringLength"] = numeric.length();
        output["fractionLength"] = outcome([&] { numeric.set("length", "3.5"); return JsonValue(true); });
        result["arrayLength"] = output;
    }
    {
        auto events = JSValue::array(), object = JSValue::object();
        object.set("valueOf", JSValue::function(+[](const JSValue& receiver, const Args&, const Args& captures) -> JSValue {
            log(captures.at(0), "valueOf"); return receiver;
        }, {events}));
        object.set("toString", JSValue::function(+[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            log(captures.at(0), "toString"); return "7";
        }, {events}));
        JsonValue output = JsonValue::Object{{"number", object.toNumber()}, {"numberEvents", events.toJSON().value()}};
        events.set("length", 0); output["string"] = object.toString(); output["stringEvents"] = events.toJSON().value();
        auto prototype = JSValue::object(), child = JSValue::object(prototype); child.set("label", "child");
        prototype.setPrimitiveConverter(JSValue::function(+[](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
            return args.at(0).string() + ":" + receiver.get("label").string();
        }));
        output["hookDefault"] = child.toPrimitive().string(); output["hookString"] = child.toString();
        output["hookNumber"] = JSValue(child.toNumber()).trace();
        child.setPrimitiveConverter(JSValue::function(firstCapture, {child}));
        output["hookObject"] = outcome([&] { return child.toPrimitive().trace(); });
        child.setPrimitiveConverter(1); output["hookNoncallable"] = outcome([&] { return child.toPrimitive().trace(); });
        output["nullPrototype"] = outcome([] { return JSValue(JSValue::object(nullptr).toNumber()).trace(); });
        auto array = JSValue::array(3); array.set("1", nullptr); array.set("2", 4);
        output["sparseArray"] = array.toString();
        auto self = JSValue::array(2); self.set("0", self); self.set("1", 5); output["cyclicArray"] = self.toString();
        output["emptyArrayNumber"] = JSValue::array().toNumber(); output["objectString"] = JSValue::object().toString();
        output["nativeString"] = JSValue::array().get("join").toString();
        result["coercion"] = output;
    }
    {
        auto object = JSValue::object(); Descriptor getter;
        getter.getter = JSValue::function(+[](const JSValue& receiver, const Args&, const Args&) -> JSValue {
            auto target = receiver; target.set("added", 3); target.erase("later"); return 1;
        }); getter.enumerable = true; object.defineProperty("first", getter); object.set("later", 2);
        JsonValue output = JsonValue::Object{{"getterSnapshot", object.toJSON().value()}};
        auto child = JSValue::object(); child.set("toJSON", JSValue::function(+[](const JSValue&, const Args& args, const Args&) -> JSValue { return args.at(0); }));
        output["toJSONRoot"] = child.toJSON().value(); object.set("child", child); output["toJSONChild"] = object.toJSON().value();
        auto array = JSValue::array(2);
        getter.getter = JSValue::function(+[](const JSValue& receiver, const Args&, const Args&) -> JSValue {
            auto target = receiver; target.set("2", 3); return 1;
        }); array.defineProperty("0", getter); output["arraySnapshot"] = array.toJSON().value(); output["arrayLengthAfter"] = array.length();
        auto fn = JSValue::function(firstCapture, {1}); auto fnArray = JSValue::array(1); fnArray.set("0", fn);
        output["functionArray"] = fnArray.toJSON().value();
        result["json"] = output;
    }
    {
        const JSValue scalar("\xf0\x9f\x92\xa9");
        const auto high = scalar.get("0"), low = scalar.get("1");
        auto object = JSValue::object();
        object.set(scalar.string(), 1); object.set(high.string() + low.string(), 2);
        result["utf16"] = JsonValue::Object{{"length", scalar.get("length").number()},
            {"identity", scalar.strictEquals(JSValue(high.string() + low.string()))},
            {"high", high.toJSON().value()}, {"low", low.toJSON().value()},
            {"joined", JSValue(high.string() + low.string()).toJSON().value()},
            {"keys", names(object.keys())}, {"keyValue", object.get(scalar.string()).number()},
            {"erase", object.erase(high.string() + low.string())}, {"afterErase", object.hasOwn(scalar.string())}};
    }
    return result;
}
std::string unhex(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); i += 2) result += static_cast<char>(std::stoul(text.substr(i, 2), nullptr, 16));
    return result;
}
}

int main() {
    try {
        JsonValue result = JsonValue::Object{{"scenarios", scenarios()}};
        JsonValue::Array conversions;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            if (line[0] == 'N') {
                const auto number = std::bit_cast<double>(std::stoull(line.substr(1), nullptr, 16));
                conversions.emplace_back(JSValue(number).toString());
            } else if (line[0] == 'S') conversions.push_back(JSValue(JSValue(unhex(line.substr(1))).toNumber()).trace());
            else throw std::invalid_argument("Unknown conversion input");
        }
        result["conversions"] = std::move(conversions);
        std::cout << result.dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

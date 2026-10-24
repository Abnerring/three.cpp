#include "catch.hpp"
#include "core/JSValue.h"
#include "core/JSDataSnapshot.h"
#include "materials/MeshBasicMaterial.h"
#include <cmath>
#include <limits>

TEST_CASE("Array includes uses SameValueZero and reads sparse inherited values", "[core][runtime][includes]") {
    auto values = JSValue::array(4); const auto includes = values.get("includes");
    const auto nan = JSValue(std::numeric_limits<double>::quiet_NaN());
    auto item = JSValue::object(); values.set("0", nan); values.set("1", -0.0); values.set("3", item);
    REQUIRE(includes.get("name").strictEquals("includes")); REQUIRE(includes.get("length").strictEquals(1));
    REQUIRE(includes.call(values, {nan}).strictEquals(true));
    REQUIRE(includes.call(values, {0}).strictEquals(true));
    REQUIRE(includes.call(values, {JSValue{}}).strictEquals(true));
    REQUIRE(includes.call(values, {item}).strictEquals(true));
    REQUIRE(includes.call(values, {JSValue::object()}).strictEquals(false));
    REQUIRE_FALSE(values.hasOwn("2"));
    auto prototype = JSValue::object(values.getPrototype()); prototype.set("2", "inherited"); values.setPrototype(prototype);
    REQUIRE(includes.call(values, {JSValue{}}).strictEquals(false));
    REQUIRE(includes.call(values, {"inherited"}).strictEquals(true));
    REQUIRE(includes.call(values, {item, -1.8}).strictEquals(true));
    REQUIRE(includes.call(values, {nan, 1}).strictEquals(false));
    REQUIRE(includes.call(values, {nan, -std::numeric_limits<double>::infinity()}).strictEquals(true));
    REQUIRE(includes.call(values, {nan, std::numeric_limits<double>::infinity()}).strictEquals(false));
    REQUIRE(includes.call("hello", {"e"}).strictEquals(true));
    REQUIRE(includes.call(7, {JSValue{}}).strictEquals(false));
    REQUIRE_THROWS_AS(includes.call(nullptr), JSTypeError); REQUIRE_THROWS_AS(includes.call(JSValue{}), JSTypeError);
    REQUIRE_THROWS_AS(includes.call(values, {nan, JSValue::symbol("start")}), JSTypeError);
}
TEST_CASE("Array includes snapshots length before fromIndex conversion and ordered Get", "[core][runtime][includes]") {
    using Args = std::vector<JSValue>;
    auto object = JSValue::object(), events = JSValue::array(), length = JSValue::object(), start = JSValue::object();
    length.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
        captures[0].get("push").call(captures[0], {"length." + args[0].toString()}); return JSValue(3);
    }, {events}));
    JSPropertyDescriptor getter; getter.configurable = true;
    getter.getter = JSValue::function([](const JSValue&, const Args&, const Args& captures) {
        captures[0].get("push").call(captures[0], {"length.get"}); return captures[1];
    }, {events, length}); object.defineProperty("length", getter);
    start.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
        captures[0].get("push").call(captures[0], {"start." + args[0].toString()});
        auto object = captures[1]; JSPropertyDescriptor changed; changed.value = 0; changed.writable = true; changed.configurable = true;
        object.defineProperty("length", changed); return JSValue(1.8);
    }, {events, object}));
    for (int index : {1, 2}) {
        JSPropertyDescriptor element;
        element.getter = JSValue::function([](const JSValue&, const Args&, const Args& captures) {
            captures[0].get("push").call(captures[0], {"get" + captures[1].toString()}); return captures[1];
        }, {events, index}); object.defineProperty(std::to_string(index), element);
    }
    const auto includes = JSValue::array().get("includes");
    REQUIRE(includes.call(object, {2, start}).strictEquals(true));
    REQUIRE(events.toJSON()->dump() == "[\"length.get\",\"length.number\",\"start.number\",\"get1\",\"get2\"]");
    REQUIRE(object.get("length").strictEquals(0));
    auto throwing = JSValue::object(); throwing.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSRangeError("fromIndex"); }));
    REQUIRE(includes.call(object, {2, throwing}).strictEquals(false));
    auto longObject = JSValue::object(); longObject.set("length", 9007199254740991.0); longObject.set("9007199254740990", "last");
    REQUIRE(includes.call(longObject, {"last", -1}).strictEquals(true));
    REQUIRE(includes.call(longObject, {"last", 9007199254740991.0}).strictEquals(false));
    auto nonempty = JSValue::array(1); REQUIRE_THROWS_AS(includes.call(nonempty, {2, throwing}), JSRangeError);
}

TEST_CASE("Bound functions retain receivers and forward construction with bound arguments", "[core][runtime]") {
    auto receiver = JSValue::object(); receiver.set("value", 7);
    auto callback = JSValue::function([](const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        return self.get("value").toNumber() + args[0].toNumber() + args[1].toNumber();
    }, {}, "sum", 2);
    const auto bound = callback.get("bind").call(callback, {receiver, 3});
    REQUIRE(bound.call(nullptr, {5}).strictEquals(15));
    REQUIRE(bound.get("name").strictEquals("bound sum")); REQUIRE(bound.get("length").strictEquals(1));
    REQUIRE_FALSE(bound.hasOwn("prototype")); REQUIRE_FALSE(bound.isConstructible());
    auto constructor = JSValue::constructor([](const JSValue& receiver, const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
        auto self = receiver; self.set("x", args[0]); self.set("y", args[1]); return {};
    }, {}, "Pair", 2);
    auto boundConstructor = constructor.bind(receiver, {11});
    const auto pair = boundConstructor.construct({12});
    REQUIRE(pair.getPrototype().sameValue(constructor.get("prototype")));
    REQUIRE(pair.get("x").strictEquals(11)); REQUIRE(pair.get("y").strictEquals(12));
    REQUIRE_FALSE(receiver.hasOwn("x"));
    REQUIRE_THROWS_AS(boundConstructor.call({}, {12}), JSTypeError);
}

TEST_CASE("Native data snapshots retain numbers without invoking stringify hooks", "[core][runtime][serialization]") {
    auto data=JSValue::object(),events=JSValue::array();
    const auto numbers=JSValue::fromJSON(JsonValue::Array{-0.0,std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()});
    data.set("numbers",numbers);
    data.set("toJSON",JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,
                                           const std::vector<JSValue>& captures)->JSValue {
        auto events=captures[0];events.set("0","stringify");return "replacement";
    },{events}));
    const auto snapshot=snapshotData(data);
    REQUIRE(events.length()==0);
    const auto& raw=snapshot->at("numbers").get<JsonValue::Array>();
    REQUIRE(std::signbit(raw[0].get<double>()));
    REQUIRE(std::isnan(raw[1].get<double>()));
    REQUIRE(raw[2].get<double>()==std::numeric_limits<double>::infinity());
    REQUIRE(raw[3].get<double>()==-std::numeric_limits<double>::infinity());
    REQUIRE_FALSE(snapshot->contains("toJSON"));
    REQUIRE(data.toJSON()->get<std::string>()=="replacement");
    REQUIRE(events.length()==1);
    const auto stringified=numbers.toJSON();
    REQUIRE_FALSE(std::signbit((*stringified)[0].get<double>()));
    REQUIRE((*stringified)[1].isNull());
    REQUIRE((*stringified)[2].isNull());
    REQUIRE((*stringified)[3].isNull());
    data.set("self",data);
    REQUIRE_THROWS_AS(snapshotData(data),JSTypeError);
    REQUIRE(data.toJSON()->get<std::string>()=="replacement");
}

TEST_CASE("Runtime distinguishes missing undefined null NaN and signed zero", "[core][runtime]") {
    JSValue missing, null(nullptr), nan(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(missing.isUndefined());
    REQUIRE_FALSE(missing.strictEquals(null));
    REQUIRE_FALSE(nan.strictEquals(nan));
    REQUIRE(JSValue(-0.0).strictEquals(JSValue(0.0)));
    REQUIRE(JSValue(-0.0).trace().at("value") == JsonValue("-0"));
    REQUIRE_FALSE(missing.toBoolean());
    REQUIRE_FALSE(null.toBoolean());
    REQUIRE_FALSE(nan.toBoolean());
    REQUIRE_FALSE(JSValue(-0.0).toBoolean());
    REQUIRE_FALSE(JSValue("").toBoolean());
    REQUIRE(JSValue("0").toBoolean());
    REQUIRE(JSValue::object().toBoolean());
    REQUIRE_FALSE(JSValue(1).strictEquals(JSValue("1")));
}

TEST_CASE("Runtime object aliases and enumeration retain JavaScript semantics", "[core][runtime]") {
    auto object = JSValue::object();
    object.set("b", 1);
    object.set("10", 10);
    object.set("2", 2);
    object.set("01", 1);
    object.set("a", JSValue{});
    auto alias = object;
    REQUIRE(alias.strictEquals(object));
    REQUIRE_FALSE(JSValue::object().strictEquals(object));
    alias.set("b", 9);
    REQUIRE(object.get("b").number() == 9);
    const std::vector<std::string> initial{"2", "10", "b", "01", "a"};
    REQUIRE(object.keys() == initial);
    REQUIRE(object.get("absent").isUndefined());
    REQUIRE_FALSE(object.hasOwn("absent"));
    REQUIRE(object.hasOwn("a"));
    object.erase("b");
    object.set("b", 3);
    const std::vector<std::string> reinserted{"2", "10", "01", "a", "b"};
    REQUIRE(object.keys() == reinserted);
    REQUIRE_THROWS_AS(JSValue{}.get("x"), std::invalid_argument);
    REQUIRE_THROWS_AS(JSValue(nullptr).get("x"), std::invalid_argument);
}

TEST_CASE("Runtime sparse arrays preserve holes and array index limits", "[core][runtime]") {
    auto array = JSValue::array(3);
    array.set("1", JSValue{});
    array.set("tag", "keep");
    REQUIRE(array.length() == 3);
    REQUIRE_FALSE(array.hasOwn("0"));
    REQUIRE(array.hasOwn("1"));
    REQUIRE(array.get("0").isUndefined());
    REQUIRE(array.get("1").isUndefined());
    REQUIRE(array.hasOwn("length"));
    REQUIRE_FALSE(array.erase("length"));
    array.set("5", 5);
    REQUIRE(array.length() == 6);
    array.set("length", 2);
    REQUIRE_FALSE(array.hasOwn("5"));
    REQUIRE(array.hasOwn("tag"));
    array.set("4294967295", "not an index");
    REQUIRE(array.length() == 2);
    array.set("length", -0.0);
    REQUIRE(array.length() == 0);
    REQUIRE_THROWS_AS(array.set("length", -1), std::out_of_range);
    REQUIRE_THROWS_AS(array.set("length", 1.5), std::out_of_range);
}

TEST_CASE("Runtime serialization handles aliases holes undefined and cycles", "[core][runtime]") {
    REQUIRE_FALSE(JSValue{}.toJSON().has_value());
    auto object = JSValue::object();
    auto child = JSValue::object();
    child.set("value", 1);
    object.set("first", child);
    object.set("second", child);
    object.set("missing", JSValue{});
    object.set("infinite", std::numeric_limits<double>::infinity());
    child.set("value", 2);
    auto json = object.toJSON().value();
    REQUIRE_FALSE(json.contains("missing"));
    REQUIRE(json.at("infinite").isNull());
    REQUIRE(json.at("first").at("value") == JsonValue(2));
    REQUIRE(json.at("second").at("value") == JsonValue(2));
    auto array = JSValue::array(3);
    array.set("1", JSValue{});
    array.set("2", std::numeric_limits<double>::quiet_NaN());
    REQUIRE(array.toJSON().value().dump() == "[null,null,null]");
    child.set("parent", object);
    REQUIRE_THROWS_AS(object.toJSON(), std::invalid_argument);
    const auto trace = object.trace();
    const auto& properties = trace.at("properties").get<JsonValue::Array>();
    REQUIRE(properties[1][1].at("type") == JsonValue("reference"));
}

TEST_CASE("Runtime collection releases unreachable cycles and retains live aliases", "[core][runtime]") {
    auto anchor = JSValue::object();
    anchor.collectGarbage();
    const auto initial = anchor.heapObjectCount();
    {
        auto first = JSValue::object(), second = JSValue::object();
        first.set("next", second);
        second.set("next", first);
        REQUIRE(anchor.heapObjectCount() == initial + 2);
        REQUIRE(anchor.collectGarbage() == 0);
        anchor.set("saved", first);
    }
    REQUIRE(anchor.collectGarbage() == 0);
    auto saved = anchor.get("saved");
    anchor.erase("saved");
    REQUIRE(anchor.collectGarbage() == 0);
    saved = JSValue{};
    REQUIRE(anchor.collectGarbage() == 2);
    REQUIRE(anchor.heapObjectCount() == initial);
}

TEST_CASE("JSON preserves key order and material userData assignment shares identity", "[core][runtime]") {
    JsonValue data = JsonValue::Object{{"z", 1}, {"10", 10}, {"2", 2}, {"a", 3}};
    REQUIRE(data.dump() == "{\"2\":2,\"10\":10,\"z\":1,\"a\":3}");
    const auto runtime = JSValue::fromJSON(data);
    REQUIRE(runtime.toJSON().value().dump() == data.dump());
    JsonValue parameters = JsonValue::Object{{"userData", data}};
    MeshBasicMaterial material;
    material.setValues(parameters);
    parameters["userData"]["z"] = 8;
    REQUIRE(material.userData.at("z") == JsonValue(8));
    auto serialized = material.toJSON();
    material.userData["z"] = 9;
    REQUIRE(serialized.at("userData").at("z") == JsonValue(9));
    auto cloned = material.clone();
    material.userData["z"] = 10;
    REQUIRE(cloned->userData.at("z") == JsonValue(9));
}

TEST_CASE("Runtime moved and temporary handles preserve roots and release user records", "[core][runtime]") {
    std::size_t realmBaseline = 0;
    {
        auto anchor = JSValue::object();
        anchor.collectGarbage();
        const auto baseline = anchor.heapObjectCount();
        realmBaseline = baseline;
        auto object = JSValue::object();
        object.set("self", object);
        auto moved = std::move(object);
        REQUIRE(object.isUndefined());
        REQUIRE(anchor.collectGarbage() == 0);
        auto saved = moved.get("self");
        moved = JSValue{};
        REQUIRE(anchor.collectGarbage() == 0);
        REQUIRE(saved.get("self").strictEquals(saved));
        saved = JSValue{};
        REQUIRE(anchor.collectGarbage() == 1);
        REQUIRE(anchor.heapObjectCount() == baseline);
        anchor.set("self", anchor);
    }
    // Module prototype handles may retain this realm between test cases, but
    // they must not retain the old user object's managed self edge.
    auto fresh = JSValue::object();
    fresh.collectGarbage();
    // Built-in methods can grow without changing the ownership invariant:
    // replacing the sole user object must leave the realm at its initial size.
    REQUIRE(fresh.heapObjectCount() == realmBaseline);
}

namespace {
JSValue capturedFirst(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
    return captures.at(0);
}
JSValue receiverRead(const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    return receiver.get("stored");
}
JSValue receiverWrite(const JSValue& receiver, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    auto target = receiver;
    target.set("stored", args.at(0));
    return {};
}
JSValue countingConversion(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
    auto counter = captures.at(0);
    counter.set("calls", counter.get("calls").toNumber() + 1);
    return 2;
}
JSValue* resetHandle = nullptr;
JSPropertyDescriptor* changedDescriptor = nullptr;
JSValue reentrantLength(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
    if (resetHandle) *resetHandle = JSValue{};
    if (changedDescriptor) { changedDescriptor->value = 4; changedDescriptor->writable = false; }
    auto counter = captures.at(0);
    counter.set("calls", counter.get("calls").toNumber() + 1);
    counter.collectGarbage();
    return 2;
}
}

TEST_CASE("Runtime functions preserve identity receiver arguments and explicit captures", "[core][runtime]") {
    auto captured = JSValue::object();
    captured.set("n", 12);
    auto function = JSValue::function(capturedFirst, {captured}, "captured", 0);
    auto alias = function;
    REQUIRE(function.isCallable());
    REQUIRE(function.isObject());
    REQUIRE(alias.strictEquals(function));
    REQUIRE_FALSE(JSValue::function(capturedFirst, {captured}).strictEquals(function));
    REQUIRE(function.call().strictEquals(captured));
    REQUIRE(function.keys().empty());
    REQUIRE((function.ownPropertyNames() == std::vector<std::string>{"length", "name"}));
    REQUIRE(function.get("name").string() == "captured");
    REQUIRE_FALSE(function.toJSON());
    REQUIRE_THROWS_AS(JSValue(1).call(), JSTypeError);
    const auto inspect = JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>& args,
                                             const std::vector<JSValue>&) -> JSValue {
        return receiver.toString() + ":" + args.at(0).toString() + ":" + std::to_string(args.size());
    });
    REQUIRE(inspect.call(nullptr, {3, 4}).string() == "null:3:2");
    REQUIRE(inspect.call(JSValue{}, {3}).string() == "undefined:3:1");
}

TEST_CASE("Runtime inherited accessors use the original or explicit receiver", "[core][runtime]") {
    auto prototype = JSValue::object(), child = JSValue::object(prototype), other = JSValue::object();
    prototype.set("stored", 1); child.set("stored", 10); other.set("stored", 20);
    JSPropertyDescriptor accessor;
    accessor.getter = JSValue::function(receiverRead);
    accessor.setter = JSValue::function(receiverWrite);
    accessor.enumerable = true;
    accessor.configurable = true;
    prototype.defineProperty("value", accessor);
    REQUIRE(child.has("value"));
    REQUIRE_FALSE(child.hasOwn("value"));
    REQUIRE(child.get("value").number() == 10);
    REQUIRE(prototype.get("value", other).number() == 20);
    child.set("value", 11);
    REQUIRE(child.get("stored").number() == 11);
    REQUIRE(prototype.get("stored").number() == 1);
    REQUIRE(prototype.reflectSet("value", 21, other));
    REQUIRE(other.get("stored").number() == 21);
    REQUIRE_FALSE(child.hasOwn("value"));
    REQUIRE(child.keys() == std::vector<std::string>{"stored"});
    REQUIRE_FALSE(prototype.setPrototype(child));
    child.preventExtensions();
    REQUIRE_FALSE(child.setPrototype(nullptr));
    REQUIRE(child.setPrototype(prototype));
    REQUIRE_FALSE(child.reflectSet("new", 3, child));
}

TEST_CASE("Runtime descriptors preserve SameValue and inherited readonly restrictions", "[core][runtime]") {
    auto object = JSValue::object();
    JSPropertyDescriptor frozen;
    frozen.value = -0.0;
    object.defineProperty("zero", frozen);
    REQUIRE(object.keys().empty());
    REQUIRE_FALSE(object.erase("zero"));
    JSPropertyDescriptor update;
    update.value = 0.0;
    REQUIRE_FALSE(object.defineOwnProperty("zero", update));
    update.value = -0.0;
    REQUIRE(object.defineOwnProperty("zero", update));
    frozen.value = std::numeric_limits<double>::quiet_NaN();
    object.defineProperty("nan", frozen);
    update.value = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(object.defineOwnProperty("nan", update));
    auto child = JSValue::object(object);
    REQUIRE_FALSE(child.reflectSet("zero", 2, child));
    REQUIRE_FALSE(child.hasOwn("zero"));
    REQUIRE_THROWS_AS(child.set("zero", 2), JSTypeError);
    JSPropertyDescriptor invalid;
    invalid.getter = nullptr;
    REQUIRE_THROWS_AS(object.defineOwnProperty("bad", invalid), JSTypeError);
    invalid.getter = JSValue::function(receiverRead);
    invalid.value = 2;
    REQUIRE_THROWS_AS(object.defineProperty("bad", invalid), JSTypeError);
}

TEST_CASE("Runtime array length conversion and partial shrink follow ArraySetLength", "[core][runtime]") {
    auto array = JSValue::array();
    array.set("0", 0); array.set("2", 2); array.set("4", 4);
    JSPropertyDescriptor fixedIndex;
    fixedIndex.configurable = false;
    array.defineProperty("2", fixedIndex);
    JSPropertyDescriptor shrink;
    shrink.value = 1;
    shrink.writable = false;
    REQUIRE_FALSE(array.defineOwnProperty("length", shrink));
    REQUIRE(array.length() == 3);
    REQUIRE_FALSE(array.hasOwn("4"));
    REQUIRE(array.hasOwn("2"));
    REQUIRE_FALSE(*array.getOwnPropertyDescriptor("length")->writable);
    REQUIRE_FALSE(array.reflectSet("3", 3, array));
    auto numeric = JSValue::array(4), counter = JSValue::object(), length = JSValue::object();
    counter.set("calls", 0);
    length.set("valueOf", JSValue::function(countingConversion, {counter}));
    numeric.set("length", length);
    REQUIRE(numeric.length() == 2);
    REQUIRE(counter.get("calls").number() == 2);
    numeric.set("length", "3");
    REQUIRE(numeric.length() == 3);
    REQUIRE_THROWS_AS(numeric.set("length", "3.5"), JSRangeError);
}

TEST_CASE("Runtime coercion uses JavaScript numeric grammar formatting and array joining", "[core][runtime]") {
    REQUIRE(std::isnan(JSValue{}.toNumber()));
    REQUIRE(JSValue(nullptr).toNumber() == 0);
    REQUIRE(JSValue("  ").toNumber() == 0);
    REQUIRE(JSValue("0x20000000000001").toNumber() == 9007199254740992.0);
    REQUIRE(JSValue("0b11").toNumber() == 3);
    REQUIRE(JSValue("0o17").toNumber() == 15);
    REQUIRE(JSValue(std::string(10000, '9') + "e-10000").toNumber() == 1);
    REQUIRE(JSValue("0." + std::string(10000, '0') + "1e10001").toNumber() == 1);
    const std::string halfway = "1.00000000000000011102230246251565404236316680908203125";
    REQUIRE(JSValue(halfway).toNumber() == 1);
    REQUIRE(JSValue(halfway + std::string(1300, '0') + "1").toNumber() == std::nextafter(1.0, 2.0));
    REQUIRE(std::signbit(JSValue("-0").toNumber()));
    REQUIRE(std::isnan(JSValue("+0x1").toNumber()));
    REQUIRE(std::isnan(JSValue("inf").toNumber()));
    REQUIRE(std::isnan(JSValue("0x1p2").toNumber()));
    REQUIRE(JSValue(-0.0).toString() == "0");
    REQUIRE(JSValue(1e-6).toString() == "0.000001");
    REQUIRE(JSValue(1e-7).toString() == "1e-7");
    REQUIRE(JSValue(1e20).toString() == "100000000000000000000");
    REQUIRE(JSValue(1e21).toString() == "1e+21");
    REQUIRE(JSValue(std::numeric_limits<double>::denorm_min()).toString() == "5e-324");
    auto array = JSValue::array(3);
    array.set("1", nullptr); array.set("2", 4);
    REQUIRE(array.toString() == ",,4");
    REQUIRE(std::isnan(array.toNumber()));
    auto self = JSValue::array(2);
    self.set("0", self); self.set("1", 5);
    REQUIRE(self.toString() == ",5");
    REQUIRE(JSValue::array().toNumber() == 0);
    REQUIRE(JSValue::object().toString() == "[object Object]");
    REQUIRE_THROWS_AS(JSValue::object(nullptr).toNumber(), JSTypeError);
    auto source = JSValue::array().get("join");
    REQUIRE(source.toString() == "function join() { [native code] }");
}

TEST_CASE("Runtime conversion hooks inherit and propagate exceptions and bad returns", "[core][runtime]") {
    auto prototype = JSValue::object(), child = JSValue::object(prototype);
    prototype.setPrimitiveConverter(JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>& args,
                                                         const std::vector<JSValue>&) -> JSValue {
        return args.at(0).string() + ":" + receiver.get("label").string();
    }));
    child.set("label", "child");
    REQUIRE(child.toPrimitive().string() == "default:child");
    REQUIRE(child.toString() == "string:child");
    REQUIRE(std::isnan(child.toNumber()));
    child.setPrimitiveConverter(JSValue::function(capturedFirst, {child}));
    REQUIRE_THROWS_AS(child.toPrimitive(), JSTypeError);
    child.setPrimitiveConverter(1);
    REQUIRE_THROWS_AS(child.toPrimitive(), JSTypeError);
    child.setPrimitiveConverter(JSValue{});
    REQUIRE(child.toString() == "[object Object]");
    child.set("valueOf", JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>&) -> JSValue {
        throw JSRangeError("conversion failure");
    }));
    REQUIRE_THROWS_AS(child.toNumber(), JSRangeError);
}

TEST_CASE("Runtime GC traces prototype accessor capture and converter edges", "[core][runtime]") {
    auto anchor = JSValue::object();
    anchor.collectGarbage();
    const auto baseline = anchor.heapObjectCount();
    {
        auto prototype = JSValue::object(), child = JSValue::object(prototype), payload = JSValue::object();
        auto getter = JSValue::function(capturedFirst, {payload});
        payload.set("child", child);
        JSPropertyDescriptor descriptor;
        descriptor.getter = getter;
        descriptor.configurable = true;
        prototype.defineProperty("value", descriptor);
        child.setPrimitiveConverter(getter);
        anchor.set("root", child);
        REQUIRE(anchor.heapObjectCount() == baseline + 4);
    }
    REQUIRE(anchor.collectGarbage() == 0);
    auto child = anchor.get("root");
    auto returned = child.get("value");
    REQUIRE(returned.get("child").strictEquals(child));
    anchor.erase("root");
    child = JSValue{};
    REQUIRE(anchor.collectGarbage() == 0);
    returned = JSValue{};
    REQUIRE(anchor.collectGarbage() == 4);
    REQUIRE(anchor.heapObjectCount() == baseline);
}

TEST_CASE("Runtime executing callbacks remain rooted when their property is detached", "[core][runtime]") {
    auto owner = JSValue::object(), payload = JSValue::object();
    payload.set("value", 42);
    owner.set("method", JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>&,
                                             const std::vector<JSValue>& captures) -> JSValue {
        auto target = receiver;
        target.erase("method");
        target.collectGarbage();
        return captures.at(0).get("value");
    }, {payload}));
    payload = JSValue{};
    REQUIRE(owner.get("method").call(owner).number() == 42);
    REQUIRE_FALSE(owner.hasOwn("method"));
    REQUIRE(owner.collectGarbage() == 2);
}

TEST_CASE("Runtime JSON executes toJSON and getters with snapshot keys and array length", "[core][runtime]") {
    auto object = JSValue::object();
    JSPropertyDescriptor getter;
    getter.getter = JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>&,
                                        const std::vector<JSValue>&) -> JSValue {
        auto target = receiver;
        target.set("added", 3);
        target.erase("later");
        return 1;
    });
    getter.enumerable = true;
    object.defineProperty("first", getter);
    object.set("later", 2);
    REQUIRE(object.toJSON()->dump() == "{\"first\":1}");
    auto child = JSValue::object();
    child.set("toJSON", JSValue::function(+[](const JSValue&, const std::vector<JSValue>& args,
                                             const std::vector<JSValue>&) -> JSValue { return args.at(0); }));
    REQUIRE(child.toJSON()->dump() == "\"\"");
    object.set("child", child);
    REQUIRE(object.toJSON()->at("child") == JsonValue("child"));
    auto array = JSValue::array(2);
    getter.getter = JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>&,
                                        const std::vector<JSValue>&) -> JSValue {
        auto target = receiver;
        target.set("2", 3);
        return 1;
    });
    array.defineProperty("0", getter);
    REQUIRE(array.toJSON()->dump() == "[1,null]");
    REQUIRE(array.length() == 3);
}

TEST_CASE("Runtime UTF16 identity canonicalizes paired surrogates in values and keys", "[core][runtime]") {
    const JSValue scalar("\xf0\x9f\x92\xa9");
    const auto high = scalar.get("0"), low = scalar.get("1");
    REQUIRE(scalar.get("length").number() == 2);
    REQUIRE(scalar.strictEquals(JSValue(high.string() + low.string())));
    REQUIRE(high.toJSON()->dump() == "\"\\ud83d\"");
    REQUIRE(low.toJSON()->dump() == "\"\\udca9\"");
    REQUIRE(JSValue(high.string() + low.string()).toJSON()->dump() == scalar.toJSON()->dump());
    auto object = JSValue::object();
    object.set(scalar.string(), 1);
    object.set(high.string() + low.string(), 2);
    REQUIRE(object.keys().size() == 1);
    REQUIRE(object.get(scalar.string()).number() == 2);
    REQUIRE(object.hasOwn(high.string() + low.string()));
    REQUIRE(object.erase(high.string() + low.string()));
    REQUIRE_FALSE(object.hasOwn(scalar.string()));
    REQUIRE_THROWS_AS(JSValue(std::string("\xc0\x80", 2)), JSTypeError);
}

TEST_CASE("Runtime array coercion snapshots descriptors and roots the target across reentry", "[core][runtime]") {
    auto array = JSValue::array(3), counter = JSValue::object(), length = JSValue::object();
    counter.set("calls", 0);
    length.set("valueOf", JSValue::function(reentrantLength, {counter}));
    JSPropertyDescriptor descriptor;
    descriptor.value = length;
    changedDescriptor = &descriptor;
    REQUIRE(array.defineOwnProperty("length", descriptor));
    changedDescriptor = nullptr;
    REQUIRE(array.length() == 2);
    REQUIRE(*array.getOwnPropertyDescriptor("length")->writable);
    REQUIRE(counter.get("calls").number() == 2);
    counter.collectGarbage();
    descriptor = JSPropertyDescriptor{};
    descriptor.value = length;
    resetHandle = &array;
    REQUIRE(array.defineOwnProperty("length", descriptor));
    resetHandle = nullptr;
    REQUIRE(array.isUndefined());
    REQUIRE(counter.get("calls").number() == 4);
    REQUIRE(counter.collectGarbage() == 1);
}

TEST_CASE("Runtime conversion and trace keep original identity if callbacks reset its C++ handle", "[core][runtime]") {
    auto value = JSValue::object();
    value.set("valueOf", JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>&,
                                              const std::vector<JSValue>&) -> JSValue {
        *resetHandle = JSValue{};
        receiver.collectGarbage();
        return receiver;
    }));
    value.set("toString", JSValue::function(capturedFirst, {"7"}));
    resetHandle = &value;
    REQUIRE(value.toNumber() == 7);
    resetHandle = nullptr;
    REQUIRE(value.isUndefined());
    value = JSValue::object();
    JSPropertyDescriptor descriptor;
    descriptor.getter = JSValue::function(+[](const JSValue& receiver, const std::vector<JSValue>&,
                                             const std::vector<JSValue>&) -> JSValue {
        *resetHandle = JSValue{};
        receiver.collectGarbage();
        return 1;
    });
    descriptor.enumerable = true;
    value.defineProperty("first", descriptor);
    value.set("second", 2);
    resetHandle = &value;
    auto trace = value.trace();
    resetHandle = nullptr;
    REQUIRE(value.isUndefined());
    const auto& properties = trace.at("properties").get<JsonValue::Array>();
    REQUIRE(properties.size() == 2);
    REQUIRE(properties[0][1].at("value") == JsonValue(1));
    REQUIRE(properties[1][1].at("value") == JsonValue(2));
}

#include "geometries/Geometries.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "animation/AnimationUtils.h"
#include "loaders/ObjectLoaderShapes.h"
#include "core/BufferGeometry.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue trace = JSValue::array();
JSValue list(const Args& values = {}) {
    auto a = JSValue::array();
    for (const auto& v : values) a.get("push").call(a, {v});
    return a;
}
JSValue encode(const JSValue& value) {
    if (value.isUndefined()) return list({"undefined"});
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.toNumber();
        if (std::isnan(n)) return list({"number", "NaN"});
        if (!std::isfinite(n)) return list({"number", n < 0 ? "-Infinity" : "Infinity"});
        if (n == 0 && std::signbit(n)) return list({"number", "-0"});
    }
    if (!value.isObject()) return value;
    auto out = value.type() == JSValue::Type::Array ? JSValue::array() : JSValue::object();
    for (const auto& key : value.keys()) out.set(key, encode(value.get(key)));
    return out;
}
JSValue decode(const JSValue& value) {
    if (!value.isObject()) return value;
    if (value.hasOwn("$geometry")) {
        const auto spec = value.get("$geometry");
        const auto args = decode(spec.get("args")); Args input;
        for (unsigned i = 0; i < args.length(); ++i) input.push_back(args.get(JSValue(i)));
        return nativeGeometriesBinding(spec.get("name").toString()).construct(input);
    }
    if (value.hasOwn("$shape")) {
        const auto result = nativeObjectLoaderShapeConstructor().construct();
        return result.get("fromJSON").call(result, {decode(value.get("$shape"))});
    }
    if (value.hasOwn("$curve")) {
        const auto data = decode(value.get("$curve"));
        const auto result = nativeObjectLoaderCurveConstructors().get(data.get("type")).construct();
        return result.get("fromJSON").call(result, {data});
    }
    if (value.hasOwn("$coerce")) {
        auto object = JSValue::object();
        auto conversion = JSValue::function([](const JSValue& self, const Args&, const Args& c) {
            c[0].get("push").call(c[0], {list({"valueOf", c[1], self.strictEquals(c[2])})});
            return c[3];
        }, {trace, value.get("$coerce"), object, decode(value.get("value"))}, "valueOf", 0);
        auto getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) {
            c[0].get("push").call(c[0], {list({"get:valueOf", c[1], self.strictEquals(c[2])})});
            return c[3];
        }, {trace, value.get("$coerce"), object, conversion});
        JSPropertyDescriptor d; d.getter = getter; d.configurable = true; object.defineProperty("valueOf", d);
        return object;
    }
    if (value.hasOwn("$undefined")) return {};
    if (value.hasOwn("$number")) {
        const auto s = value.get("$number").toString();
        if (s == "NaN") return std::numeric_limits<double>::quiet_NaN();
        if (s == "Infinity") return std::numeric_limits<double>::infinity();
        if (s == "-Infinity") return -std::numeric_limits<double>::infinity();
        if (s == "-0") return -0.;
    }
    auto out = value.type() == JSValue::Type::Array ? JSValue::array() : JSValue::object();
    for (const auto& key : value.keys()) out.set(key, decode(value.get(key)));
    return out;
}
Args values(const JSValue& value) {
    Args out;
    for (unsigned i = 0; i < value.length(); ++i) out.push_back(value.get(JSValue(i)));
    return out;
}
JSValue copyArray(const JSValue& source) {
    auto out = JSValue::array();
    const auto length = source.get("length").toNumber();
    for (double i = 0; i < length; ++i) out.set(JSValue(i), encode(source.get(JSValue(i))));
    return out;
}
std::optional<std::string> parameterAlias(const JSValue& value, const JSValue& parameters) {
    if (!value.isObject()) return {};
    for (const auto& key : parameters.keys()) if (value.strictEquals(parameters.get(key))) return key;
    return {};
}
JSValue encodeJSON(const JSValue& value, const JSValue& parameters) {
    if (const auto alias = parameterAlias(value, parameters)) return list({"parameter", *alias});
    if (!value.isObject()) return encode(value);
    auto out = value.type() == JSValue::Type::Array ? JSValue::array() : JSValue::object();
    for (const auto& key : value.keys()) out.set(key, encodeJSON(value.get(key), parameters));
    return out;
}
void jsonKeyOrder(const JSValue& value, const std::string& path, JSValue out, const JSValue& parameters) {
    if (!value.isObject()) return;
    if (parameterAlias(value, parameters)) return;
    auto keys = JSValue::array();
    for (const auto& key : value.keys()) keys.get("push").call(keys, {key});
    out.get("push").call(out, {list({path, keys})});
    for (const auto& key : value.keys()) jsonKeyOrder(value.get(key), path + "/" + key, out, parameters);
}
JSValue snapshot(const JSValue& geometry) {
    auto out = JSValue::object(), attributes = JSValue::object();
    out.set("type", geometry.get("type"));
    out.set("index", geometry.get("index").isNull() ? JSValue(nullptr) : copyArray(geometry.get("index").get("array")));
    const auto source = geometry.get("attributes");
    auto names = source.keys(); std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        const auto a = source.get(name); auto item = JSValue::object();
        item.set("itemSize", a.get("itemSize")); item.set("normalized", a.get("normalized"));
        item.set("array", copyArray(a.get("array"))); attributes.set(name, item);
    }
    out.set("attributes", attributes); out.set("groups", encode(geometry.get("groups")));
    auto parameters = JSValue::object();
    for (const auto& key : geometry.get("parameters").keys()) {
        const auto v = geometry.get("parameters").get(key);
        if (!v.isObject() || v.type() == JSValue::Type::Array) parameters.set(key, encode(v));
    }
    out.set("parameters", parameters);
    return out;
}
JSValue run(const JSValue& request) {
    const auto constructor = nativeGeometriesBinding(request.get("name").toString());
    if (request.get("mode").strictEquals("copyNull")) {
        auto prototype = nativeBufferGeometryConstructor().get("prototype");
        const auto original = *prototype.getOwnPropertyDescriptor("copy");
        auto replacement = original;
        replacement.value = JSValue::function([](const JSValue& self, const Args&, const Args& c) {
            c[0].get("push").call(c[0], {list({"base", self.isNull()})}); return self;
        }, {trace}, "copy", 1);
        prototype.defineProperty("copy", replacement);
        auto source = JSValue::object(), parameters = JSValue::object();
        JSPropertyDescriptor d; d.configurable = d.enumerable = true;
        d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
            c[0].get("push").call(c[0], {"get:width"}); return 2;
        }, {trace}); parameters.defineProperty("width", d);
        d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) {
            c[0].get("push").call(c[0], {list({"get:parameters", self.strictEquals(c[1])})}); return c[2];
        }, {trace, source, parameters}); source.defineProperty("parameters", d);
        auto out = JSValue::object();
        try { constructor.get("prototype").get("copy").call(nullptr, {source}); out.set("error", nullptr); }
        catch (...) { out.set("error", nativeExceptionValue(std::current_exception()).get("name")); }
        prototype.defineProperty("copy", original); out.set("trace", trace); return out;
    }
    if (request.get("mode").strictEquals("metadata")) {
        auto out = JSValue::object(), methods = JSValue::object();
        out.set("name", constructor.get("name")); out.set("length", constructor.get("length"));
        out.set("source", constructor.get("toString").call(constructor));
        const auto proto = constructor.get("prototype");
        auto keys = list(); for (const auto& name : proto.ownPropertyNames()) keys.get("push").call(keys, {name});
        out.set("prototypeNames", keys);
        for (const auto& name : proto.ownPropertyNames()) if (name != "constructor") {
            const auto d = *proto.getOwnPropertyDescriptor(name); auto row = JSValue::object();
            row.set("name", d.value->get("name")); row.set("length", d.value->get("length"));
            row.set("source", d.value->get("toString").call(*d.value));
            row.set("flags", list({*d.writable, *d.enumerable, *d.configurable})); methods.set(name, row);
        }
        out.set("methods", methods);
        if (constructor.hasOwn("fromJSON")) {
            const auto f = constructor.get("fromJSON"); auto row = JSValue::object();
            row.set("name", f.get("name")); row.set("length", f.get("length"));
            row.set("source", f.get("toString").call(f)); out.set("fromJSON", row);
        }
        return out;
    }
    const auto args = values(decode(request.get("args")));
    auto data = decode(request.get("data"));
    if (request.get("mode").strictEquals("fromJSONTrace")) {
        for (const auto& key : data.keys()) {
            const auto value = data.get(key);
            JSPropertyDescriptor d; d.enumerable = d.configurable = true;
            d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c) {
                c[0].get("push").call(c[0], {list({c[1], self.strictEquals(c[2])})});
                return c[3];
            }, {trace, key, data, value});
            data.defineProperty(key, d);
        }
    }
    const auto result = request.get("mode").strictEquals("fromJSON") || request.get("mode").strictEquals("fromJSONTrace")
        ? constructor.get("fromJSON").call(constructor, {data, request.get("shapes").isUndefined() ? JSValue::object() : decode(request.get("shapes"))})
        : constructor.construct(args);
    if (request.get("mode").strictEquals("toJSON")) {
        auto geometry = result; geometry.set("uuid", "geometry-under-test");
        const auto json = geometry.get("toJSON").call(geometry);
        auto out = JSValue::object(), order = JSValue::array();
        const auto parameters = geometry.get("parameters");
        out.set("json", encodeJSON(json, parameters)); jsonKeyOrder(json, "", order, parameters); out.set("keyOrder", order); return out;
    }
    if (request.get("mode").strictEquals("copy")) {
        const auto target = constructor.construct();
        auto out = JSValue::object(), identity = JSValue::object();
        out.set("returnsTarget", target.get("copy").call(target, {result}).strictEquals(target));
        out.set("geometry", snapshot(target));
        out.set("parametersDistinct", !target.get("parameters").strictEquals(result.get("parameters")));
        for (const auto& key : result.get("parameters").keys()) identity.set(key,
            target.get("parameters").get(key).strictEquals(result.get("parameters").get(key)));
        out.set("parameterAliases", identity); return out;
    }
    auto out = snapshot(result);
    if (request.get("mode").strictEquals("fromJSONTrace")) {
        auto row = JSValue::object(); row.set("geometry", out); row.set("trace", trace); return row;
    }
    if (request.get("mode").strictEquals("trace")) {
        auto identity = JSValue::object();
        const auto keys = request.get("parameterKeys");
        for (unsigned i = 0; i < keys.length(); ++i)
            identity.set(keys.get(JSValue(i)), result.get("parameters").get(keys.get(JSValue(i))).strictEquals(args.at(i)));
        auto row = JSValue::object(); row.set("geometry", out); row.set("trace", trace); row.set("identity", identity); return row;
    }
    return out;
}
}

int main(int argc, char** argv) {
    try {
        const auto input = argc == 2 ? std::string(argv[1]) : std::string(std::istreambuf_iterator<char>(std::cin), {});
        std::cout << run(Animation_detail::parseJSON(input)).toJSON()->dump() << '\n';
    } catch (...) {
        const auto error = nativeExceptionValue(std::current_exception()); auto out = JSValue::object();
        out.set("error", error.isObject() ? error.get("name") : JSValue("thrown-value"));
        out.set("message", error.isObject() ? error.get("message") : error);
        std::cout << out.toJSON()->dump() << '\n'; return 1;
    }
}

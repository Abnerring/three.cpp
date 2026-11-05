#pragma once

#include "nodes/functions/BSDF/BRDF_EON.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>
#include <vector>

namespace BRDFEONScenarios {
using Args = std::vector<JSValue>;
inline JSValue object() { return JSValue::object(nullptr); }
inline void push(JSValue array, const JSValue& value) { array.set(array.length(), value); }
inline JSValue call(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
inline JSValue node(const char* type, const Args& args) { return nativeTSLBaseBinding(type).call({}, args); }
inline const std::vector<std::string>& fields() {
    static const std::vector<std::string> values{"nodeType", "value", "method", "op", "aNode", "bNode", "cNode",
        "node", "nodes", "condNode", "ifNode", "elseNode", "convertTo", "components", "global",
        "shaderNode", "rawInputs", "jsFunc", "layout", "once"};
    return values;
}
class Graph {
    Args seen_;
    JSValue objects_ = JSValue::array();
    JSValue encode(const JSValue& value) {
        if (value.sameValue(nativeNormalBinding("normalView"))) return "@normalView";
        if (value.sameValue(nativePositionBinding("positionViewDirection"))) return "@positionViewDirection";
        if (value.isUndefined()) return "@undefined";
        if (value.isCallable()) return call(value, "toString");
        if (value.type() == JSValue::Type::String) {
            const auto text = value.string();
            if (text.size() > 2 && text.starts_with("fn") &&
                std::all_of(text.begin() + 2, text.end(), [](unsigned char c) { return std::isdigit(c); })) return "@function";
        }
        if (!value.isObject()) return value;
        for (unsigned i = 0; i < seen_.size(); ++i)
            if (value.sameValue(seen_[i])) { auto ref = object(); ref.set("ref", i); return ref; }
        const auto index = seen_.size(); seen_.push_back(value);
        auto ref = object(), row = object(), properties = JSValue::array();
        ref.set("ref", index); row.set("properties", properties); push(objects_, row);
        if (value.type() == JSValue::Type::Array) {
            row.set("kind", "array");
            for (unsigned i = 0; i < value.length(); ++i) push(properties, encode(value.get(JSValue(i))));
        } else {
            const bool isNode = value.get("isNode").strictEquals(true);
            row.set("kind", isNode ? value.get("type") : JSValue("object"));
            const auto keys = isNode ? fields() : value.keys();
            for (const auto& key : keys) if (value.getOwnPropertyDescriptor(key)) {
                auto entry = JSValue::array(); push(entry, key); push(entry, encode(value.get(key))); push(properties, entry);
            }
        }
        return ref;
    }
  public:
    JSValue operator()(const JSValue& value) {
        auto out = object(); out.set("root", encode(value)); out.set("objects", objects_); return out;
    }
};
inline JSValue inputs(bool defaults = false) {
    auto out = object();
    out.set("lightDirection", node("vec3", {0.2, 0.7, 0.6}));
    out.set("diffuseColor", node("vec3", {0.5, 0.4, 0.3})); out.set("roughness", node("float", {0.35}));
    if (!defaults) {
        out.set("normalView", node("vec3", {0.0, 0.0, 1.0}));
        out.set("viewDirection", node("vec3", {0.1, 0.2, 0.97}));
    }
    return out;
}
inline JSValue directionalInputs(double roughness = .35, double dotNV = .6) {
    auto out = object(); out.set("diffuseColor", node("vec3", {1.2, -.2, .5}));
    out.set("roughness", node("float", {roughness})); out.set("dotNV", node("float", {dotNV})); return out;
}
inline JSValue callback(const JSValue& fn) { return fn.get("shaderNode").get("jsFunc"); }
inline JSValue findFON(const JSValue& value, Args& visited) {
    if (!value.isObject() || value.isCallable()) return {};
    for (const auto& previous : visited) if (value.sameValue(previous)) return {};
    visited.push_back(value);
    if (value.getOwnPropertyDescriptor("jsFunc")) {
        const auto fn = value.get("jsFunc");
        if (call(fn, "toString").string().find("const muComp = mu.oneMinus()") != std::string::npos) return fn;
    }
    const auto keys = value.get("isNode").strictEquals(true) ? fields() : value.keys();
    for (const auto& key : keys) if (value.getOwnPropertyDescriptor(key)) {
        const auto found = findFON(value.get(key), visited); if (found.isCallable()) return found;
    }
    return {};
}
inline JSValue numeric(double value) {
    auto out = object(); out.set("value", value);
    for (const auto* method : {"oneMinus", "mul", "add", "sub"})
        out.set(method, JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                const auto left = self.get("value").toNumber();
                if (captures[0].strictEquals("oneMinus")) return numeric(1 - left);
                const auto right = args[0].isObject() ? args[0].get("value").toNumber() : args[0].toNumber();
                return numeric(captures[0].strictEquals("mul") ? left * right : captures[0].strictEquals("add") ? left + right : left - right);
            }, {method}, method, std::string(method) == "oneMinus" ? 0 : 1));
    return out;
}
inline JSValue fonValues() {
    const auto result = callback(nativeEONDirectionalAlbedo()).call({}, {directionalInputs()});
    Args visited; const auto fn = findFON(result, visited);
    if (!fn.isCallable()) throw std::logic_error("FON callback missing from EON graph");
    auto out = JSValue::array();
    for (const auto mu : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        auto input = object(); input.set("mu", numeric(mu)); input.set("roughness", numeric(.65)); input.set("A", numeric(.7));
        push(out, fn.call({}, {input}).get("value"));
    }
    return out;
}
inline JSValue metadata(const JSValue& fn) {
    auto out = object(); const auto shader = fn.get("shaderNode"), source = callback(fn);
    const auto chainLength = fn.get("length");
    auto length = object(); length.set("kind", chainLength.isCallable() ? "function" : "value");
    if (chainLength.isCallable()) {
        length.set("name", chainLength.get("name")); length.set("length", chainLength.get("length"));
        length.set("source", call(chainLength, "toString"));
    } else length.set("value", chainLength);
    out.set("name", fn.get("name")); out.set("length", length); out.set("isFn", fn.get("isFn"));
    out.set("source", call(source, "toString")); out.set("callbackLength", source.get("length"));
    const auto layout = shader.get("layout");
    if (layout.isObject()) { auto d = object(); d.set("type", layout.get("type")); d.set("inputs", layout.get("inputs")); out.set("layout", d); }
    else out.set("layout", nullptr);
    return out;
}
inline JSValue all() {
    auto out = object(), exports = JSValue::array(), graphs = JSValue::array();
    for (const auto& key : nativeBRDFEONModule().keys()) push(exports, key);
    out.set("exports", exports); out.set("default", metadata(nativeBRDFEON()));
    out.set("directional", metadata(nativeEONDirectionalAlbedo()));
    push(graphs, Graph()(callback(nativeBRDFEON()).call({}, {inputs()})));
    push(graphs, Graph()(callback(nativeBRDFEON()).call({}, {inputs(true)})));
    for (const auto& values : std::vector<std::pair<double, double>>{{0, -.3}, {1e-8, 0}, {.35, .6}, {1, 1.4}})
        push(graphs, Graph()(callback(nativeEONDirectionalAlbedo()).call({}, {directionalInputs(values.first, values.second)})));
    out.set("graphs", graphs); out.set("fonValues", fonValues()); return out;
}
} // namespace BRDFEONScenarios

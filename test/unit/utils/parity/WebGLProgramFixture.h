#pragma once
#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "renderers/webgl/WebGLProgram.h"
#include "utils.h"
#include <cmath>
#include <string>

namespace ProgramFixture {
using Args = std::vector<JSValue>;
inline JSValue arg(const Args& a, std::size_t i) {
    return i < a.size() ? a[i] : JSValue();
}
inline void data(JSValue o, const std::string& k, const JSValue& v) {
    JSPropertyDescriptor d;
    d.value = v;
    d.writable = d.enumerable = d.configurable = true;
    o.defineProperty(k, d);
}
inline JSValue list(const Args& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i)
        data(result, std::to_string(i), values[i]);
    return result;
}
inline JSValue encode(const JSValue& v, unsigned depth = 0) {
    if (v.isUndefined())
        return "@undefined";
    if (v.isSymbol())
        return "@symbol";
    if (v.type() == JSValue::Type::Number) {
        if (std::isnan(v.number()))
            return "@NaN";
        if (!std::isfinite(v.number()))
            return v.number() > 0 ? "@Infinity" : "@-Infinity";
        if (v.number() == 0 && std::signbit(v.number()))
            return "@-0";
        return v;
    }
    if (v.isCallable())
        return "@function:" + v.get("name").toString() + ":" + v.get("length").toString();
    if (!v.isObject())
        return v;
    if (v.hasOwn("fixtureHandle"))
        return v.get("fixtureHandle");
    if (depth > 5)
        return "@object";
    auto result = v.type() == JSValue::Type::Array ? JSValue::array() : JSValue::object();
    for (const auto& key : v.keys())
        data(result, key, encode(v.get(key), depth + 1));
    return result;
}
inline void event(JSValue state, const Args& values) {
    auto trace = state.get("trace"), entry = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i)
        data(entry, std::to_string(i), encode(values[i]));
    data(trace, std::to_string(trace.length()), entry);
    if (trace.length() == state.get("config").get("failAt").toNumber())
        throw JSRangeError("event");
}
inline JSValue caught() {
    try {
        throw;
    } catch (const JSThrownValue& e) {
        return e.value.get("name");
    } catch (const JSRangeError&) {
        return "RangeError";
    } catch (const JSTypeError&) {
        return "TypeError";
    } catch (const std::exception&) {
        return "Error";
    }
}
inline JSValue exceptionCall(const JSValue& fn, const JSValue& receiver, const Args& args = {}) {
    try {
        return encode(fn.call(receiver, args));
    } catch (...) {
        return list({"throw", caught()});
    }
}
inline JSValue handle(const std::string& name) {
    auto result = JSValue::object();
    data(result, "fixtureHandle", name);
    return result;
}
struct RestoreProperties {
    struct Entry {
        JSValue object;
        std::string key;
        std::optional<JSPropertyDescriptor> descriptor;
    };
    std::vector<Entry> entries;
    void save(const JSValue& object, const std::string& key) {
        entries.push_back({object, key, object.getOwnPropertyDescriptor(key)});
    }
    ~RestoreProperties() {
        for (auto i = entries.rbegin(); i != entries.rend(); ++i) {
            if (i->descriptor)
                i->object.defineProperty(i->key, *i->descriptor);
            else
                i->object.erase(i->key);
        }
    }
};
inline JSValue sourceProtocol(const JSValue& state, const char* label) {
    auto result = JSValue::object();
    JSPropertyDescriptor d;
    d.configurable = d.enumerable = true;
    d.getter = JSValue::function(
        [](const JSValue& self, const Args&, const Args& c) {
            event(c[0], {"source-get-replace", c[1]});
            return JSValue::constructor(
                [](const JSValue& self, const Args& args, const Args& c) {
                    const auto regex = arg(args, 0), replacement = arg(args, 1);
                    event(c[0], {"source-replace", c[1], self.sameValue(c[2]), regex.get("source"),
                                 regex.get("flags"), replacement});
                    return self;
                },
                {c[0], c[1], self}, "replace", 2, JSValue::ConstructorKind::Callable);
        },
        {state, label});
    result.defineProperty("replace", d);
    result.setPrimitiveConverter(JSValue::function(
        [](const JSValue&, const Args& args, const Args& c) {
            event(c[0], {"source-convert", c[1], arg(args, 0)});
            return JSValue("source:" + c[1].toString());
        },
        {state, label}));
    return result;
}
inline JSValue defaults() {
    auto p = JSValue::object();
    for (const auto* name :
         {"numSunLights", "numDirLights", "numSpotLights", "numSpotLightMaps",
          "numSpotLightShadows", "numSpotLightShadowsWithMaps", "numRectAreaLights",
          "numPointLights", "numHemiLights", "numSunLightShadows", "numDirLightShadows",
          "numPointLightShadows", "numClippingPlanes", "numClipIntersection"})
        data(p, name, 0);
    data(p, "precision", "highp");
    data(p, "shaderType", "FixtureMaterial");
    data(p, "shaderName", "fixture");
    data(p, "vertexShader", "void main() { gl_Position = vec4( 0.0 ); }");
    data(p, "fragmentShader", "void main() { gl_FragColor = vec4( 1.0 ); }");
    data(p, "toneMapping", 0);
    data(p, "outputColorSpace", "srgb");
    data(p, "envMap", false);
    data(p, "isRawShaderMaterial", true);
    data(p, "rendererExtensionParallelShaderCompile", false);
    data(p, "defines", JSValue::object());
    return p;
}
inline JSValue scenario(const JSValue& config) {
    RestoreProperties restore;
    auto state = JSValue::object(), trace = JSValue::array(), results = JSValue::array(),
         gl = JSValue::object(), renderer = JSValue::object(), binding = JSValue::object();
    data(state, "trace", trace);
    data(state, "config", config);
    data(state, "shaderCount", 0);
    data(state, "programCount", 0);
    data(state, "readyCount", 0);
    data(state, "acted", false);
    data(state, "gl", gl);
    data(state, "renderer", renderer);
    data(state, "binding", binding);
    data(state, "sources", JSValue::object());
    auto parameters = defaults();
    const auto overrides = config.get("parameters");
    for (const auto& key : overrides.keys())
        data(parameters, key, overrides.get(key));
    const auto undefinedKeys = config.get("undefinedParameters");
    if (undefinedKeys.isObject())
        for (unsigned i = 0; i < undefinedKeys.length(); ++i)
            parameters.erase(undefinedKeys.get(std::to_string(i)).toString());
    data(state, "parameters", parameters);
    if (config.get("sourceProtocol").toBoolean()) {
        data(parameters, "vertexShader", sourceProtocol(state, "vertex"));
        data(parameters, "fragmentShader", sourceProtocol(state, "fragment"));
    }
    const auto sequences = config.get("parameterSequences");
    if (sequences.isObject())
        for (const auto& name : sequences.keys()) {
            auto sequenceState = JSValue::object();
            data(sequenceState, "n", 0);
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    auto counter = c[3];
                    const auto n = counter.get("n").toNumber();
                    counter.set("n", n + 1);
                    event(c[0], {"sequence", c[1], n});
                    return c[2].get(JSValue(n));
                },
                {state, name, sequences.get(name), sequenceState});
            parameters.defineProperty(name, d);
        }
    if (config.get("traceParameters").toBoolean()) {
        const auto names = config.get("parameterKeys");
        for (unsigned i = 0; i < names.length(); ++i) {
            const auto name = names.get(std::to_string(i));
            const auto value = parameters.get(name);
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"parameter", c[1]});
                    return c[2];
                },
                {state, name, value});
            parameters.defineProperty(name, d);
        }
    }
    auto debug = JSValue::object();
    data(debug, "checkShaderErrors", config.get("checkShaderErrors"));
    data(renderer, "debug", debug);
    if (config.get("shaderErrorCallback").toBoolean())
        data(debug, "onShaderError",
             JSValue::function(
                 [](const JSValue& self, const Args& args, const Args& c) {
                     event(c[0], {"onShaderError", self.sameValue(c[1]), arg(args, 1), arg(args, 2),
                                  arg(args, 3)});
                     return JSValue();
                 },
                 {state, debug}));
    data(renderer, "getContext",
         JSValue::function(
             [](const JSValue& self, const Args&, const Args& c) {
                 event(c[0], {"getContext", self.sameValue(c[1])});
                 return c[2];
             },
             {state, renderer, gl}));
    data(binding, "releaseStatesOfProgram",
         JSValue::function(
             [](const JSValue& self, const Args& args, const Args& c) {
                 event(c[0], {"release", self.sameValue(c[1]), arg(args, 0).get("id")});
                 return JSValue();
             },
             {state, binding}));
    for (const auto& [name, value] :
         std::vector<std::pair<std::string, int>>{{"VERTEX_SHADER", 35633},
                                                  {"FRAGMENT_SHADER", 35632},
                                                  {"LINK_STATUS", 35714},
                                                  {"VALIDATE_STATUS", 35715},
                                                  {"COMPILE_STATUS", 35713},
                                                  {"ACTIVE_ATTRIBUTES", 35721},
                                                  {"ACTIVE_UNIFORMS", 35718},
                                                  {"FLOAT_MAT2", 35674},
                                                  {"FLOAT_MAT3", 35675},
                                                  {"FLOAT_MAT4", 35676}})
        data(gl, name, value);
    const std::vector<std::string> commands = {
        "createProgram",     "createShader",        "shaderSource",       "compileShader",
        "attachShader",      "bindAttribLocation",  "linkProgram",        "getProgramInfoLog",
        "getShaderInfoLog",  "getProgramParameter", "getShaderParameter", "getShaderSource",
        "getError",          "deleteShader",        "deleteProgram",      "getActiveAttrib",
        "getAttribLocation", "getActiveUniform",    "getUniformLocation"};
    for (const auto& name : commands) {
        auto fn = JSValue::function(
            [](const JSValue& self, const Args& args, const Args& c) -> JSValue {
                auto s = c[0];
                const auto cfg = s.get("config"), gl = s.get("gl");
                const auto name = c[1].toString();
                Args values{name, self.sameValue(gl)};
                values.insert(values.end(), args.begin(), args.end());
                event(s, values);
                if (name == "createProgram") {
                    const auto n = s.get("programCount").toNumber() + 1;
                    s.set("programCount", n);
                    return cfg.get("nullProgram").toBoolean()
                               ? JSValue(nullptr)
                               : handle("program:" + JSValue(n).toString());
                }
                if (name == "createShader") {
                    const auto n = s.get("shaderCount").toNumber() + 1;
                    s.set("shaderCount", n);
                    return handle("shader:" + JSValue(n).toString());
                }
                if (name == "shaderSource") {
                    auto sources = s.get("sources");
                    data(sources, arg(args, 0).get("fixtureHandle").toString(), arg(args, 1));
                    return {};
                }
                if (name == "getProgramInfoLog")
                    return cfg.hasOwn("programLog") ? cfg.get("programLog") : JSValue("");
                if (name == "getShaderInfoLog") {
                    const auto field = arg(args, 0).get("fixtureHandle").strictEquals("shader:1")
                                           ? "vertexLog"
                                           : "fragmentLog";
                    return cfg.hasOwn(field) ? cfg.get(field) : JSValue("");
                }
                if (name == "getProgramParameter") {
                    const auto p = arg(args, 1);
                    if (p.strictEquals(35714))
                        return cfg.hasOwn("linkStatus") ? cfg.get("linkStatus") : JSValue(true);
                    if (p.strictEquals(35715))
                        return cfg.hasOwn("validateStatus") ? cfg.get("validateStatus")
                                                            : JSValue(true);
                    if (p.strictEquals(35721))
                        return cfg.get("attributes").isObject()
                                   ? JSValue(cfg.get("attributes").length())
                                   : JSValue(0);
                    if (p.strictEquals(35718))
                        return cfg.get("uniforms").isObject()
                                   ? JSValue(cfg.get("uniforms").length())
                                   : JSValue(0);
                    if (p.strictEquals(0x91B1)) {
                        const auto n = s.get("readyCount").toNumber();
                        s.set("readyCount", n + 1);
                        const auto values = cfg.get("readyValues");
                        return values.isObject() ? values.get(JSValue(n)) : JSValue(true);
                    }
                    return {};
                }
                if (name == "getShaderParameter")
                    return cfg.hasOwn("compileStatus") ? cfg.get("compileStatus") : JSValue(true);
                if (name == "getShaderSource")
                    return s.get("sources").get(arg(args, 0).get("fixtureHandle"));
                if (name == "getError")
                    return cfg.hasOwn("error") ? cfg.get("error") : JSValue(0);
                if (name == "getActiveAttrib")
                    return cfg.get("attributes").get(arg(args, 1));
                if (name == "getAttribLocation")
                    return "attribute:" + arg(args, 1).toString();
                if (name == "getActiveUniform")
                    return cfg.get("uniforms").get(arg(args, 1));
                if (name == "getUniformLocation")
                    return "uniform:" + arg(args, 1).toString();
                return {};
            },
            {state, name}, name, 0);
        if (config.get("traceCommands").toBoolean()) {
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"command", c[1]});
                    return c[2];
                },
                {state, name, fn});
            gl.defineProperty(name, d);
        } else
            data(gl, name, fn);
    }
    ThreeUtils::setConsoleFunction(JSValue::function(
        [](const JSValue&, const Args& args, const Args& c) {
            Args values{"console"};
            values.insert(values.end(), args.begin(), args.end());
            event(c[0], values);
            return JSValue();
        },
        {state}));
    if (config.get("arrayProtocol").toBoolean()) {
        const auto prototype = JSValue::array().getPrototype();
        for (const auto* name : {"filter", "join"}) {
            restore.save(prototype, name);
            const auto original = prototype.get(name);
            auto replacement = JSValue::constructor(
                [](const JSValue& self, const Args& args, const Args& c) {
                    event(c[0], {"array-call", c[1], self.get("length"), arg(args, 0)});
                    if (c[1].strictEquals("filter") &&
                        c[0].get("config").get("mutateAtFilter").toBoolean()) {
                        auto p = c[0].get("parameters");
                        data(p, "precision", "lowp");
                        data(p, "shaderName", "changed-filter");
                    }
                    return c[2].call(self, args);
                },
                {state, name, original}, name, 1, JSValue::ConstructorKind::Callable);
            JSPropertyDescriptor d;
            d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"array-get", c[1]});
                    return c[2];
                },
                {state, name, replacement});
            auto target = prototype;
            target.defineProperty(name, d);
        }
    }
    if (config.get("stringProtocol").toBoolean()) {
        const auto prototype = nativeStringPrototype();
        for (const auto* name : {"replace", "split", "trim", "toUpperCase"}) {
            restore.save(prototype, name);
            const auto original = prototype.get(name);
            auto method = JSValue::constructor(
                [](const JSValue& self, const Args& args, const Args& c) {
                    auto first = arg(args, 0);
                    if (first.isObject() && first.has("source"))
                        first = list({first.get("source"), first.get("flags")});
                    event(c[0], {"string-call", c[1], self, first, arg(args, 1)});
                    const auto returns = c[0].get("config").get("stringReturns");
                    if (returns.isObject() && returns.hasOwn(c[1]))
                        return returns.get(c[1]);
                    return c[2].call(self, args);
                },
                {state, name, original}, name, original.get("length").toNumber(),
                JSValue::ConstructorKind::Callable);
            JSPropertyDescriptor d;
            d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"string-get", c[1]});
                    return c[2];
                },
                {state, name, method});
            auto target = prototype;
            target.defineProperty(name, d);
        }
    }
    if (config.get("constructorSetters").toBoolean()) {
        const auto prototype = nativeWebGLProgramConstructor().get("prototype");
        for (const auto* name : {"getUniforms", "getAttributes", "isReady", "destroy", "type",
                                 "name", "id", "program"}) {
            restore.save(prototype, name);
            JSPropertyDescriptor d;
            d.configurable = true;
            d.setter = JSValue::function(
                [](const JSValue& self, const Args& args, const Args& c) {
                    event(c[0], {"constructor-set", c[1], arg(args, 0)});
                    if (c[1].strictEquals("getUniforms") &&
                        c[0].get("config").get("earlyUniforms").toBoolean()) {
                        try {
                            const auto value = arg(args, 0).call(self);
                            event(c[0], {"early-return", value.isObject()});
                        } catch (...) {
                            const auto error = caught();
                            event(c[0], {"early-throw", error});
                            if (c[0].get("config").get("earlyRethrow").toBoolean())
                                throw;
                        }
                    }
                    data(self, c[1].toString(), arg(args, 0));
                    return JSValue();
                },
                {state, name});
            auto target = prototype;
            target.defineProperty(name, d);
        }
    }
    JSValue program;
    try {
        const auto ctor = nativeWebGLProgramConstructor();
        const auto mode = config.get("constructMode").toNumber();
        if (mode == 1) {
            auto self = JSValue::object();
            program = ctor.call(self, {renderer, "cache", parameters, binding});
        } else if (mode == 2)
            program = ctor.call({}, {renderer, "cache", parameters, binding});
        else if (mode == 3) {
            auto target = JSValue::constructor(
                [](const JSValue&, const Args&, const Args&) {
                    throw JSRangeError("newTarget body");
                    return JSValue();
                },
                {}, "Other", 0, JSValue::ConstructorKind::Callable);
            program = ctor.construct({renderer, "cache", parameters, binding}, target);
        } else
            program = ctor.construct({renderer, "cache", parameters, binding});
        data(state, "program", program);
    } catch (...) {
        data(results, std::to_string(results.length()), list({"constructThrow", caught()}));
    }
    if (program.isObject()) {
        const auto actions = config.get("actions");
        for (unsigned i = 0; i < actions.length(); ++i) {
            const auto name = actions.get(std::to_string(i));
            JSValue result;
            try {
                const auto fn = program.get(name);
                if (config.get("constructMethods").toBoolean())
                    result = fn.construct();
                else
                    result = fn.call(program);
                if (name.strictEquals("getUniforms")) {
                    const auto seq = result.get("seq");
                    JSValue entries = JSValue::array();
                    for (unsigned j = 0; j < seq.length(); ++j) {
                        const auto u = seq.get(std::to_string(j));
                        data(entries, std::to_string(j),
                             list({u.get("id"), u.get("setValue").get("name"),
                                   u.get("setValue").get("length")}));
                    }
                    result = entries;
                }
                data(results, std::to_string(results.length()), list({name, encode(result)}));
            } catch (...) {
                data(results, std::to_string(results.length()), list({name, "throw", caught()}));
            }
        }
    }
    ThreeUtils::setConsoleFunction({});
    auto out = JSValue::object();
    data(out, "trace", trace);
    data(out, "results", results);
    data(out, "sources", state.get("sources"));
    if (program.isObject()) {
        auto snapshot = JSValue::object();
        for (const auto* key : {"type", "name", "id", "cacheKey", "usedTimes", "program",
                                "vertexShader", "fragmentShader", "diagnostics"})
            data(snapshot, key, encode(program.get(key)));
        auto metadata = JSValue::array();
        for (const auto* key : {"getUniforms", "getAttributes", "isReady", "destroy"}) {
            const auto fn = program.get(key);
            data(metadata, std::to_string(metadata.length()),
                 list({key, fn.get("name"), fn.get("length"), fn.hasOwn("prototype"),
                       fn.isConstructible()}));
        }
        data(snapshot, "methods", metadata);
        data(out, "program", snapshot);
    }
    const auto json = out.toJSON();
    return JSValue::fromJSON(*json);
}
} // namespace ProgramFixture

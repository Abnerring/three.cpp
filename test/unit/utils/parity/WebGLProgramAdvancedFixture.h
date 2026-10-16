#pragma once
#include "WebGLProgramFixture.h"
#include "core/JSNumber.h"
#include "math/ColorManagement.h"
#include "renderers/shaders/ShaderChunk.h"

namespace ProgramAdvancedFixture {
using namespace ProgramFixture;
inline JSValue summary(const JSValue& value) {
    if (!value.isObject())
        return encode(value);
    if (value.hasOwn("seq"))
        return list({"uniforms", value.get("seq").get("length")});
    return list({"object", list([&] {
                     Args keys;
                     for (const auto& k : value.keys())
                         keys.emplace_back(k);
                     return keys;
                 }())});
}
inline JSValue metadata(const JSValue& fn) {
    auto out = JSValue::array();
    for (const auto& key : fn.ownPropertyNames()) {
        const auto d = fn.getOwnPropertyDescriptor(key);
        data(out, std::to_string(out.length()),
             list({key, d->writable.value_or(false), d->enumerable.value_or(false),
                   d->configurable.value_or(false),
                   key == "prototype" ? JSValue(fn.get(key).get("constructor").sameValue(fn))
                                      : encode(fn.get(key))}));
    }
    return out;
}
inline void reentry(const JSValue& state, const std::string& phase) {
    const auto config = state.get("config");
    if (!config.get("reentryPhase").strictEquals(phase) || state.get("entered").toBoolean())
        return;
    auto s = state;
    s.set("entered", true);
    event(state, {"enter", phase});
    try {
        if (config.get("reentryMethod").strictEquals("construct")) {
            const auto p = nativeWebGLProgramConstructor().construct(
                {state.get("renderer"), "nested", defaults(), state.get("binding")});
            event(state, {"nested", p.get("id")});
        } else {
            const auto p = state.get("program"), name = config.get("reentryMethod");
            const auto fn = p.get(name);
            event(state, {"reentry-result",
                          summary(fn.call(config.get("reentryBare").toBoolean() ? JSValue() : p))});
        }
    } catch (...) {
        event(state, {"reentry-throw", caught()});
        if (config.get("reentryRethrow").toBoolean())
            throw;
    }
}
inline JSValue scalar(const JSValue& state, const char* label, const JSValue& value) {
    auto o = JSValue::object();
    o.setPrimitiveConverter(JSValue::function(
        [](const JSValue&, const Args& a, const Args& c) {
            event(c[0], {"convert", c[1], arg(a, 0)});
            reentry(c[0], "convert:" + c[1].toString());
            return c[2];
        },
        {state, label, value}));
    return o;
}
inline JSValue scenario(const JSValue& config) {
    RestoreProperties restore;
    auto s = JSValue::object(), gl = JSValue::object(), renderer = JSValue::object(),
         binding = JSValue::object();
    data(s, "trace", JSValue::array());
    data(s, "config", config);
    data(s, "entered", false);
    data(s, "renderer", renderer);
    data(s, "gl", gl);
    data(s, "binding", binding);
    data(s, "sources", JSValue::object());
    data(s, "shaderN", 0);
    data(s, "programN", 0);
    data(s, "readyN", 0);
    data(s, "countN", 0);
    auto parameters = defaults();
    const auto overrides = config.get("parameters");
    if (overrides.isObject())
        for (const auto& k : overrides.keys())
            data(parameters, k, overrides.get(k));
    data(s, "parameters", parameters);
    data(renderer, "getContext",
         JSValue::function(
             [](const JSValue& self, const Args&, const Args& c) {
                 event(c[0], {"context", self.sameValue(c[1])});
                 return c[2];
             },
             {s, renderer, gl}));
    auto debug = JSValue::object();
    data(debug, "checkShaderErrors", config.get("debug").toBoolean());
    data(renderer, "debug", debug);
    data(s, "debug", debug);
    if (config.get("debugGetters").toBoolean()) {
        for (const auto* key : {"checkShaderErrors", "onShaderError"}) {
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"debug-get", c[1]});
                    reentry(c[0], "debug:" + c[1].toString());
                    if (c[1].strictEquals("checkShaderErrors"))
                        return JSValue(true);
                    const auto mode = c[0].get("config").get("debugMode").toNumber();
                    if (mode == 1)
                        return JSValue(17);
                    if (mode == 2) {
                        auto counter = c[0];
                        const auto n = counter.get("debugN").toNumber();
                        counter.set("debugN", std::isnan(n) ? 1 : n + 1);
                        if (n >= 1)
                            return JSValue(17);
                    }
                    return JSValue::function(
                        [](const JSValue& self, const Args& a, const Args& c) {
                            event(c[0], {"shader-error", self.sameValue(c[1]), arg(a, 1), arg(a, 2),
                                         arg(a, 3)});
                            reentry(c[0], "error-callback");
                            return JSValue();
                        },
                        {c[0], c[0].get("debug")});
                },
                {s, key});
            debug.defineProperty(key, d);
        }
        data(s, "debugN", 0);
    }
    auto release = JSValue::function(
        [](const JSValue& self, const Args& a, const Args& c) {
            event(c[0], {"release", self.sameValue(c[1]), arg(a, 0).get("id")});
            reentry(c[0], "release");
            return JSValue();
        },
        {s, binding});
    data(binding, "releaseStatesOfProgram", release);
    for (const auto& [k, v] : std::vector<std::pair<std::string, int>>{{"VERTEX_SHADER", 35633},
                                                                       {"FRAGMENT_SHADER", 35632},
                                                                       {"LINK_STATUS", 35714},
                                                                       {"VALIDATE_STATUS", 35715},
                                                                       {"COMPILE_STATUS", 35713},
                                                                       {"ACTIVE_ATTRIBUTES", 35721},
                                                                       {"ACTIVE_UNIFORMS", 35718},
                                                                       {"FLOAT_MAT2", 35674},
                                                                       {"FLOAT_MAT3", 35675},
                                                                       {"FLOAT_MAT4", 35676}})
        data(gl, k, v);
    for (const auto* k :
         {"createProgram", "createShader", "shaderSource", "compileShader", "attachShader",
          "bindAttribLocation", "linkProgram", "getProgramInfoLog", "getShaderInfoLog",
          "getProgramParameter", "getShaderParameter", "getShaderSource", "getError",
          "deleteShader", "deleteProgram", "getActiveAttrib", "getAttribLocation",
          "getActiveUniform", "getUniformLocation"}) {
        const auto fn = JSValue::function(
            [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                const auto state = c[0], config = state.get("config"), name = c[1];
                Args e{name, self.sameValue(state.get("gl"))};
                e.insert(e.end(), a.begin(), a.end());
                event(state, e);
                reentry(state, "body:" + name.toString());
                if (name.strictEquals("createProgram") || name.strictEquals("createShader")) {
                    const auto field = name.strictEquals("createProgram") ? "programN" : "shaderN";
                    auto st = state;
                    const auto n = st.get(field).toNumber() + 1;
                    st.set(field, n);
                    return handle((name.strictEquals("createProgram") ? "p" : "s") +
                                  JSValue(n).toString());
                }
                if (name.strictEquals("shaderSource")) {
                    data(state.get("sources"), arg(a, 0).get("fixtureHandle").toString(),
                         arg(a, 1));
                    return {};
                }
                if (name.strictEquals("getProgramInfoLog"))
                    return config.hasOwn("programLog") ? config.get("programLog") : JSValue("");
                if (name.strictEquals("getShaderInfoLog"))
                    return config.hasOwn("shaderLog") ? config.get("shaderLog") : JSValue("");
                if (name.strictEquals("getProgramParameter")) {
                    const auto p = arg(a, 1);
                    if (p.strictEquals(35714))
                        return config.hasOwn("link") ? config.get("link") : JSValue(true);
                    if (p.strictEquals(35715))
                        return true;
                    if (p.strictEquals(35718))
                        return 0;
                    if (p.strictEquals(35721))
                        return config.get("attributeProtocol").toBoolean()
                                   ? scalar(state, "count",
                                            config.hasOwn("attributeCount")
                                                ? config.get("attributeCount")
                                                : JSValue(1))
                                   : JSValue(0);
                    if (p.strictEquals(0x91B1)) {
                        auto st = state;
                        const auto n = st.get("readyN").toNumber();
                        st.set("readyN", n + 1);
                        const auto values = config.get("ready");
                        return values.isObject() ? values.get(JSValue(n)) : JSValue(true);
                    }
                    return {};
                }
                if (name.strictEquals("getShaderParameter"))
                    return config.hasOwn("compile") ? config.get("compile") : JSValue(true);
                if (name.strictEquals("getShaderSource"))
                    return state.get("sources").get(arg(a, 0).get("fixtureHandle"));
                if (name.strictEquals("getError"))
                    return 0;
                if (name.strictEquals("getActiveAttrib")) {
                    auto info = JSValue::object();
                    for (const auto* key : {"name", "type"}) {
                        JSPropertyDescriptor d;
                        d.enumerable = d.configurable = true;
                        d.getter = JSValue::function(
                            [](const JSValue&, const Args&, const Args& c) {
                                event(c[0], {"attrib-get", c[1]});
                                reentry(c[0], "attrib:" + c[1].toString());
                                if (c[1].strictEquals("name"))
                                    return scalar(c[0], "attribute",
                                                  c[0].get("config").hasOwn("attributeName")
                                                      ? c[0].get("config").get("attributeName")
                                                      : JSValue("position"));
                                return c[0].get("config").hasOwn("attributeType")
                                           ? c[0].get("config").get("attributeType")
                                           : JSValue(35665);
                            },
                            {state, key});
                        info.defineProperty(key, d);
                    }
                    return info;
                }
                if (name.strictEquals("getAttribLocation"))
                    return "location:" + arg(a, 1).toString();
                return {};
            },
            {s, k}, k, 0);
        JSPropertyDescriptor d;
        d.enumerable = d.configurable = true;
        d.getter = JSValue::function(
            [](const JSValue&, const Args&, const Args& c) {
                event(c[0], {"command-get", c[1]});
                reentry(c[0], "getter:" + c[1].toString());
                return c[2];
            },
            {s, k, fn});
        gl.defineProperty(k, d);
    }
    ThreeUtils::setConsoleFunction(JSValue::function(
        [](const JSValue&, const Args& a, const Args& c) {
            Args e{"console"};
            e.insert(e.end(), a.begin(), a.end());
            event(c[0], e);
            return JSValue();
        },
        {s}));
    if (config.get("colorProtocol").toBoolean()) {
        data(parameters, "isRawShaderMaterial", false);
        const auto cm = nativeColorManagement();
        for (const auto* key :
             {"_getMatrix", "getTransfer", "getLuminanceCoefficients", "workingColorSpace"}) {
            restore.save(cm, key);
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"cm-get", c[1]});
                    reentry(c[0], "cm-get:" + c[1].toString());
                    if (c[1].strictEquals("workingColorSpace"))
                        return JSValue("working");
                    return JSValue::function(
                        [](const JSValue& self, const Args& a, const Args& c) -> JSValue {
                            event(c[0],
                                  {"cm-call", c[1], self.sameValue(c[2]), arg(a, 1), arg(a, 2)});
                            reentry(c[0], "cm-call:" + c[1].toString());
                            const auto mode = c[0].get("config").get("colorMode").toNumber();
                            if (c[1].strictEquals("getTransfer"))
                                return c[0].get("config").hasOwn("transfer")
                                           ? c[0].get("config").get("transfer")
                                           : JSValue("srgb");
                            auto target = arg(a, 0);
                            if (c[1].strictEquals("getLuminanceCoefficients")) {
                                target.set("x", 0.1);
                                target.set("y", 0.2);
                                target.set("z", 0.3);
                                return target;
                            }
                            if (mode == 1)
                                target.set("elements", 17);
                            else if (mode == 2) {
                                auto elements = JSValue::object();
                                data(elements, "map",
                                     JSValue::function(
                                         [](const JSValue&, const Args& a, const Args& c) {
                                             event(c[0], {"map", arg(a, 0).get("length")});
                                             return scalar(c[0], "mapped", "matrix-custom");
                                         },
                                         {c[0]}));
                                target.set("elements", elements);
                            } else {
                                target.set("elements",
                                           list({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9}));
                            }
                            return target;
                        },
                        {c[0], c[1], c[2]});
                },
                {s, key, cm});
            auto target = cm;
            target.defineProperty(key, d);
        }
        if (config.get("fixedProtocol").toBoolean()) {
            const auto proto = nativeNumberPrototype();
            restore.save(proto, "toFixed");
            JSPropertyDescriptor d;
            d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue&, const Args&, const Args& c) {
                    event(c[0], {"fixed-get"});
                    return JSValue::function(
                        [](const JSValue& self, const Args& a, const Args& c) {
                            event(c[0], {"fixed-call", self, arg(a, 0)});
                            reentry(c[0], "fixed-call");
                            return c[0].get("config").hasOwn("fixedReturn")
                                       ? c[0].get("config").get("fixedReturn")
                                       : self;
                        },
                        {c[0]});
                },
                {s});
            auto target = proto;
            target.defineProperty("toFixed", d);
        }
    }
    if (config.get("chunkProtocol").toBoolean()) {
        const auto chunks = nativeShaderChunk();
        restore.save(chunks, "fixture_advanced");
        JSPropertyDescriptor d;
        d.enumerable = d.configurable = true;
        d.getter = JSValue::function(
            [](const JSValue&, const Args&, const Args& c) {
                event(c[0], {"chunk-get"});
                reentry(c[0], "chunk-get");
                return c[0].get("config").hasOwn("chunk") ? c[0].get("config").get("chunk")
                                                          : JSValue("chunk NUM_DIR_LIGHTS");
            },
            {s});
        auto target = chunks;
        target.defineProperty("fixture_advanced", d);
        data(parameters, "vertexShader", "#include <fixture_advanced>");
    }
    if (config.get("numericProtocol").toBoolean())
        for (const auto* key :
             {"numSpotLightShadows", "numSpotLightMaps", "numSpotLightShadowsWithMaps",
              "numClippingPlanes", "numClipIntersection", "shadowMapType", "combine", "envMapMode",
              "toneMapping"})
            data(parameters, key, scalar(s, key, config.get("numericValue")));
    auto results = JSValue::array();
    JSValue program;
    try {
        program =
            nativeWebGLProgramConstructor().construct({renderer, "advanced", parameters, binding});
        data(s, "program", program);
    } catch (...) {
        data(results, std::to_string(results.length()), list({"construct-throw", caught()}));
    }
    if (program.isObject()) {
        const auto actions = config.get("actions");
        for (unsigned i = 0; i < actions.length(); ++i) {
            const auto name = actions.get(JSValue(i));
            try {
                const auto fn = program.get(name);
                JSValue receiver = program;
                if (config.get("bare").toBoolean())
                    receiver = {};
                else if (config.get("other").toBoolean()) {
                    receiver = JSValue::object();
                    data(receiver, "id", "other");
                    data(receiver, "name", "other-name");
                    data(receiver, "type", "other-type");
                }
                const auto value =
                    config.get("constructMethod").toBoolean() ? fn.construct() : fn.call(receiver);
                data(results, std::to_string(results.length()), list({name, summary(value)}));
            } catch (...) {
                data(results, std::to_string(results.length()), list({name, "throw", caught()}));
            }
        }
    }
    ThreeUtils::setConsoleFunction({});
    auto out = JSValue::object();
    data(out, "trace", s.get("trace"));
    data(out, "results", results);
    data(out, "sources", s.get("sources"));
    if (program.isObject()) {
        data(out, "id", program.get("id"));
        data(out, "program", encode(program.get("program")));
        data(out, "diagnostics", encode(program.get("diagnostics")));
        if (config.get("metadata").toBoolean()) {
            auto m = JSValue::object();
            data(m, "constructor", metadata(nativeWebGLProgramConstructor()));
            for (const auto* k : {"getUniforms", "getAttributes", "isReady", "destroy"})
                data(m, k, metadata(program.get(k)));
            data(out, "metadata", m);
        }
    }
    return out;
}
} // namespace ProgramAdvancedFixture

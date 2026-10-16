// Direct port of three.js WebGLProgram.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLProgram.h"
#include "constants.h"
#include "core/JSModuleObject.h"
#include "core/JSNumber.h"
#include "core/JSPromise.h"
#include "core/JSRegExp.h"
#include "core/JSString.h"
#include "core/NativeObject.h"
#include "math/ColorManagement.h"
#include "math/Matrix3.h"
#include "math/Vector3.h"
#include "renderers/shaders/ShaderChunk.h"
#include "renderers/webgl/WebGLShader.h"
#include "renderers/webgl/WebGLUniforms.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <unordered_set>

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t i) {
    return i < args.size() ? args[i] : JSValue();
}
void literal(JSValue target, const std::string& key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = d.enumerable = d.configurable = true;
    target.defineProperty(key, d);
}
JSValue array(const Args& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i)
        literal(result, std::to_string(i), values[i]);
    return result;
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber(), y = b.toNumber();
    return x + y;
}
JSValue subtract(const JSValue& left, const JSValue& right) {
    const auto a = left.toNumber(), b = right.toNumber();
    return a - b;
}
bool greater(const JSValue& left, const JSValue& right) {
    // Both source operands are evaluated before abstract relational conversion.
    const auto a = left.toPrimitive(JSValue::PreferredType::Number);
    const auto b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) {
        const auto na = a.get("length").toNumber(), nb = b.get("length").toNumber();
        for (double i = 0; i < std::min(na, nb); ++i) {
            const auto k = JSValue(i).toString();
            const auto x = a.get(k).toString(), y = b.get(k).toString();
            if (x != y)
                return x > y;
        }
        return na > nb;
    }
    const auto x = a.toNumber(), y = b.toNumber();
    return x > y;
}
template <class F> JSValue callLater(const JSValue& receiver, const char* name, F arguments) {
    const auto method = receiver.get(name);
    const auto args = arguments();
    return method.call(receiver, args);
}
JSValue call(const JSValue& receiver, const char* name, Args args = {}) {
    return receiver.get(name).call(receiver, args);
}
JSValue fallback(const JSValue& value, const JSValue& other) {
    return value.toBoolean() ? value : other;
}

// Source lexical bindings have no object prototype or user-visible properties.
// Their managed environment is shared by every escaped closure.
JSValue environment() {
    return JSValue::object(nullptr);
}
void bind(JSValue env, const char* name, const JSValue& value) {
    literal(env, name, value);
}
[[noreturn]] void referenceError(const char* name) {
    auto error = JSValue::object();
    literal(error, "name", "ReferenceError");
    literal(error, "message", std::string("Cannot access '") + name + "' before initialization");
    throw JSThrownValue(error);
}
JSValue lexical(const JSValue& env, const char* name) {
    if (!env.hasOwn(name))
        referenceError(name);
    return env.get(name);
}
void assign(JSValue env, const char* name, const JSValue& value) {
    if (!env.hasOwn(name))
        referenceError(name);
    env.set(name, value);
}
// Same ordinary-object for-in policy used by the pinned BindingStates port.
// Snapshot candidates once, preserve nonenumerable shadowing, and test the
// live chain for deletion before each body execution.
template <class F> void forIn(JSValue object, F visit) {
    if (object.isNull() || object.isUndefined())
        return;
    if (!object.isObject()) {
        if (object.type() == JSValue::Type::String)
            object = createNativeStringObject(object);
        else if (object.type() == JSValue::Type::Number)
            object = createNativeNumberObject(object);
        else
            return; // Boolean/Symbol boxing is a shared runtime boundary.
    }
    std::unordered_set<std::string> visited;
    std::vector<std::string> candidates;
    for (auto level = object; !level.isNull(); level = level.getPrototype()) {
        for (const auto& name : level.ownPropertyNames()) {
            if (visited.contains(name))
                continue;
            const auto d = level.getOwnPropertyDescriptor(name);
            if (!d)
                continue;
            visited.insert(name);
            if (d->enumerable.value_or(false))
                candidates.push_back(name);
        }
    }
    for (const auto& name : candidates)
        if (object.has(name))
            visit(name);
}
double parseInt(const JSValue& value) {
    // These calls have no radix argument. Preserve ToString and the decimal/
    // 0x-prefix integer scan, including negative zero and trailing characters.
    const auto text = value.toString();
    const auto units = JSValue(text);
    const auto n = units.get("length").toNumber();
    std::size_t i = 0;
    const auto whitespace = [](const std::string& s) {
        return s == "\t" || s == "\n" || s == "\v" || s == "\f" || s == "\r" || s == " " ||
               s == "\xC2\xA0" || s == "\xE1\x9A\x80" || s == "\xE2\x80\x80" ||
               s == "\xE2\x80\x81" || s == "\xE2\x80\x82" || s == "\xE2\x80\x83" ||
               s == "\xE2\x80\x84" || s == "\xE2\x80\x85" || s == "\xE2\x80\x86" ||
               s == "\xE2\x80\x87" || s == "\xE2\x80\x88" || s == "\xE2\x80\x89" ||
               s == "\xE2\x80\x8A" || s == "\xE2\x80\xA8" || s == "\xE2\x80\xA9" ||
               s == "\xE2\x80\xAF" || s == "\xE2\x81\x9F" || s == "\xE3\x80\x80" ||
               s == "\xEF\xBB\xBF";
    };
    const auto at = [&](std::size_t j) { return units.get(std::to_string(j)).toString(); };
    while (i < n && whitespace(at(i)))
        ++i;
    bool negative = false;
    if (i < n && (at(i) == "+" || at(i) == "-")) {
        negative = at(i) == "-";
        ++i;
    }
    bool hex = false;
    if (i + 1 < n && at(i) == "0" && (at(i + 1) == "x" || at(i + 1) == "X")) {
        hex = true;
        i += 2;
    }
    std::string digits;
    for (; i < n; ++i) {
        const auto c = at(i);
        if (c.size() != 1 ||
            !((c[0] >= '0' && c[0] <= '9') ||
              (hex && ((c[0] >= 'a' && c[0] <= 'f') || (c[0] >= 'A' && c[0] <= 'F')))))
            break;
        digits += c;
    }
    if (digits.empty())
        return std::numeric_limits<double>::quiet_NaN();
    const auto numeric = (negative ? "-" : "") + (hex ? std::string("0x") : std::string()) + digits;
    return std::strtod(numeric.c_str(), nullptr);
}
JSValue handleSource(const JSValue& string, const JSValue& errorLine) {
    const auto lines = call(string, "split", {"\n"});
    auto lines2 = JSValue::array();
    const auto lower = subtract(errorLine, 6).toNumber();
    const auto from = std::isnan(lower) ? lower : std::max(lower, 0.0);
    const auto upper = add(errorLine, 6);
    const auto length = lines.get("length");
    const auto x = upper.toNumber(), y = length.toNumber();
    const auto to = (std::isnan(x) || std::isnan(y)) ? std::numeric_limits<double>::quiet_NaN()
                                                     : std::min(x, y);
    for (double i = from; i < to; ++i) {
        const JSValue line = i + 1;
        callLater(lines2, "push", [&] {
            std::string value = line.strictEquals(errorLine) ? ">" : " ";
            value += " ";
            value += line.toString();
            value += ": ";
            value += lines.get(JSValue(i)).toString();
            return Args{value};
        });
    }
    return call(lines2, "join", {"\n"});
}
JSValue getEncodingComponents(const JSValue& module, const JSValue& colorSpace) {
    const auto cm = module.get("ColorManagement"), m0 = module.get("m0");
    callLater(cm, "_getMatrix", [&] { return Args{m0, cm.get("workingColorSpace"), colorSpace}; });
    const auto values = m0.get("elements");
    const auto mapped = callLater(values, "map", [&] {
        return Args{
            JSValue::function([](const JSValue&, const Args& args,
                                 const Args&) { return call(argument(args, 0), "toFixed", {4}); },
                              {}, "", 1)};
    });
    const JSValue encodingMatrix = "mat3( " + mapped.toString() + " )";
    const auto transfer = call(cm, "getTransfer", {colorSpace});
    if (transfer.strictEquals(LinearTransfer))
        return array({encodingMatrix, "LinearTransferOETF"});
    if (transfer.strictEquals(SRGBTransfer))
        return array({encodingMatrix, "sRGBTransferOETF"});
    ThreeUtils::warn({"WebGLProgram: Unsupported color space: ", colorSpace});
    return array({encodingMatrix, "LinearTransferOETF"});
}
JSValue getShaderErrors(const JSValue& gl, const JSValue& shader, const JSValue& type) {
    const auto status =
        callLater(gl, "getShaderParameter", [&] { return Args{shader, gl.get("COMPILE_STATUS")}; });
    const auto log = fallback(call(gl, "getShaderInfoLog", {shader}), "");
    const auto errors = call(log, "trim");
    if (status.toBoolean() && errors.strictEquals(""))
        return "";
    const auto pattern = createNativeRegExp("ERROR: 0:(\\d+)", "");
    const auto matches = call(pattern, "exec", {errors});
    if (!matches.toBoolean())
        return errors;
    const auto errorLine = parseInt(matches.get("1"));
    auto result = add(call(type, "toUpperCase"), "\n\n");
    result = add(result, errors);
    result = add(result, "\n\n");
    const auto shaderSource = call(gl, "getShaderSource", {shader});
    return add(result, handleSource(shaderSource, errorLine));
}
JSValue getTexelEncodingFunction(const JSValue& module, const Args& args) {
    const auto functionName = argument(args, 0), colorSpace = argument(args, 1);
    const auto components = getEncodingComponents(module, colorSpace);
    auto lines = JSValue::array();
    literal(lines, "0", "vec4 " + functionName.toString() + "( vec4 value ) {");
    std::string line = "\treturn ";
    line += components.get("1").toString();
    line += "( vec4( value.rgb * ";
    line += components.get("0").toString();
    line += ", value.a ) );";
    literal(lines, "1", line);
    literal(lines, "2", "}");
    return call(lines, "join", {"\n"});
}
JSValue getToneMappingFunction(const JSValue& module, const Args& args) {
    const auto functionName = argument(args, 0), toneMapping = argument(args, 1);
    const auto name = module.get("toneMappingFunctions").get(toneMapping);
    if (name.isUndefined()) {
        ThreeUtils::warn({"WebGLProgram: Unsupported toneMapping:", toneMapping});
        return add(add("vec3 ", functionName),
                   "( vec3 color ) { return LinearToneMapping( color ); }");
    }
    auto result = add("vec3 ", functionName);
    result = add(result, "( vec3 color ) { return ");
    result = add(result, name);
    return add(result, "ToneMapping( color ); }");
}
JSValue getLuminanceFunction(const JSValue& module, const Args&) {
    const auto v0 = module.get("v0");
    call(module.get("ColorManagement"), "getLuminanceCoefficients", {v0});
    const auto r = call(v0.get("x"), "toFixed", {4});
    const auto g = call(v0.get("y"), "toFixed", {4});
    const auto b = call(v0.get("z"), "toFixed", {4});
    auto lines = JSValue::array();
    literal(lines, "0", "float luminance( const in vec3 rgb ) {");
    std::string weights = "\tconst vec3 weights = vec3( ";
    weights += r.toString();
    weights += ", ";
    weights += g.toString();
    weights += ", ";
    weights += b.toString();
    weights += " );";
    literal(lines, "1", weights);
    literal(lines, "2", "\treturn dot( weights, rgb );");
    literal(lines, "3", "}");
    return call(lines, "join", {"\n"});
}
JSValue generatePrecision(const JSValue&, const Args& args) {
    const auto parameters = argument(args, 0);
    std::string result = "precision ";
    constexpr const char* types[] = {"float",
                                     "int",
                                     "sampler2D",
                                     "samplerCube",
                                     "sampler3D",
                                     "sampler2DArray",
                                     "sampler2DShadow",
                                     "samplerCubeShadow",
                                     "sampler2DArrayShadow",
                                     "isampler2D",
                                     "isampler3D",
                                     "isamplerCube",
                                     "isampler2DArray",
                                     "usampler2D",
                                     "usampler3D",
                                     "usamplerCube",
                                     "usampler2DArray"};
    for (std::size_t i = 0; i < std::size(types); ++i) {
        if (i)
            result += "precision ";
        result += parameters.get("precision").toString();
        result += " ";
        result += types[i];
        result += ";\n\t";
    }
    if (parameters.get("precision").strictEquals("highp"))
        result += "\n#define HIGH_PRECISION";
    else if (parameters.get("precision").strictEquals("mediump"))
        result += "\n#define MEDIUM_PRECISION";
    else if (parameters.get("precision").strictEquals("lowp"))
        result += "\n#define LOW_PRECISION";
    return result;
}
JSValue generateVertexExtensions(const JSValue& module, const JSValue& parameters) {
    auto chunks = JSValue::array();
    literal(chunks, "0",
            parameters.get("extensionClipCullDistance").toBoolean()
                ? "#extension GL_ANGLE_clip_cull_distance : require"
                : "");
    literal(chunks, "1",
            parameters.get("extensionMultiDraw").toBoolean()
                ? "#extension GL_ANGLE_multi_draw : require"
                : "");
    const auto filtered = call(chunks, "filter", {module.get("filterEmptyLine")});
    return call(filtered, "join", {"\n"});
}
JSValue generateDefines(const JSValue& defines) {
    auto chunks = JSValue::array();
    forIn(defines, [&](const std::string& name) {
        const auto value = defines.get(name);
        if (value.strictEquals(false))
            return;
        callLater(chunks, "push", [&] { return Args{add("#define " + name + " ", value)}; });
    });
    return call(chunks, "join", {"\n"});
}
JSValue fetchAttributeLocations(const JSValue& gl, const JSValue& program) {
    auto attributes = JSValue::object();
    const auto n = callLater(gl, "getProgramParameter",
                             [&] { return Args{program, gl.get("ACTIVE_ATTRIBUTES")}; });
    for (double i = 0; i < n.toNumber(); ++i) {
        const auto info = call(gl, "getActiveAttrib", {program, i});
        const auto name = info.get("name");
        int locationSize = 1;
        {
            const auto t = info.get("type"), constant = gl.get("FLOAT_MAT2");
            if (t.strictEquals(constant))
                locationSize = 2;
        }
        {
            const auto t = info.get("type"), constant = gl.get("FLOAT_MAT3");
            if (t.strictEquals(constant))
                locationSize = 3;
        }
        {
            const auto t = info.get("type"), constant = gl.get("FLOAT_MAT4");
            if (t.strictEquals(constant))
                locationSize = 4;
        }
        auto entry = JSValue::object();
        literal(entry, "type", info.get("type"));
        literal(entry, "location", call(gl, "getAttribLocation", {program, name}));
        literal(entry, "locationSize", locationSize);
        attributes.set(name, entry);
    }
    return attributes;
}
JSValue replaceLightNums(JSValue string, const JSValue& parameters) {
    const auto shadows = parameters.get("numSpotLightShadows"),
               maps = parameters.get("numSpotLightMaps");
    const auto sum = add(shadows, maps);
    const auto overlap = parameters.get("numSpotLightShadowsWithMaps");
    const auto coords = subtract(sum, overlap);
    constexpr const char* replacements[][2] = {
        {"NUM_SUN_LIGHTS", "numSunLights"},
        {"NUM_DIR_LIGHTS", "numDirLights"},
        {"NUM_SPOT_LIGHTS", "numSpotLights"},
        {"NUM_SPOT_LIGHT_MAPS", "numSpotLightMaps"},
        {"NUM_SPOT_LIGHT_COORDS", nullptr},
        {"NUM_RECT_AREA_LIGHTS", "numRectAreaLights"},
        {"NUM_POINT_LIGHTS", "numPointLights"},
        {"NUM_HEMI_LIGHTS", "numHemiLights"},
        {"NUM_SUN_LIGHT_SHADOWS", "numSunLightShadows"},
        {"NUM_DIR_LIGHT_SHADOWS", "numDirLightShadows"},
        {"NUM_SPOT_LIGHT_SHADOWS_WITH_MAPS", "numSpotLightShadowsWithMaps"},
        {"NUM_SPOT_LIGHT_SHADOWS", "numSpotLightShadows"},
        {"NUM_POINT_LIGHT_SHADOWS", "numPointLightShadows"}};
    for (const auto& entry : replacements)
        string = callLater(string, "replace", [&] {
            const auto pattern = createNativeRegExp(entry[0], "g");
            const auto value = entry[1] ? parameters.get(entry[1]) : coords;
            return Args{pattern, value};
        });
    return string;
}
JSValue replaceClippingPlaneNums(JSValue string, const JSValue& parameters) {
    string = callLater(string, "replace", [&] {
        const auto pattern = createNativeRegExp("NUM_CLIPPING_PLANES", "g");
        return Args{pattern, parameters.get("numClippingPlanes")};
    });
    return callLater(string, "replace", [&] {
        const auto pattern = createNativeRegExp("UNION_CLIPPING_PLANES", "g");
        const auto count = parameters.get("numClippingPlanes"),
                   intersection = parameters.get("numClipIntersection");
        return Args{pattern, subtract(count, intersection)};
    });
}
JSValue resolveIncludes(const JSValue& module, const JSValue& string) {
    return callLater(string, "replace", [&] {
        return Args{module.get("includePattern"), module.get("includeReplacer")};
    });
}
JSValue unrollLoops(const JSValue& module, const JSValue& string) {
    return callLater(string, "replace", [&] {
        return Args{module.get("unrollLoopPattern"), module.get("loopReplacer")};
    });
}

#include "renderers/webgl/WebGLProgramPrefixes.inc"

void onFirstUse(const JSValue& env, const JSValue& self) {
    const auto renderer = env.get("renderer"), gl = env.get("gl"), program = env.get("program");
    const auto vertex = env.get("glVertexShader"), fragment = env.get("glFragmentShader");
    if (renderer.get("debug").get("checkShaderErrors").toBoolean()) {
        const auto programInfoLog = fallback(call(gl, "getProgramInfoLog", {program}), "");
        const auto vertexInfoLog = fallback(call(gl, "getShaderInfoLog", {vertex}), "");
        const auto fragmentInfoLog = fallback(call(gl, "getShaderInfoLog", {fragment}), "");
        const auto programLog = call(programInfoLog, "trim");
        const auto vertexLog = call(vertexInfoLog, "trim");
        const auto fragmentLog = call(fragmentInfoLog, "trim");
        bool runnable = true, haveDiagnostics = true;
        const auto status = callLater(gl, "getProgramParameter",
                                      [&] { return Args{program, gl.get("LINK_STATUS")}; });
        if (status.strictEquals(false)) {
            runnable = false;
            if (renderer.get("debug").get("onShaderError").isCallable())
                call(renderer.get("debug"), "onShaderError", {gl, program, vertex, fragment});
            else {
                const auto vertexErrors = getShaderErrors(gl, vertex, "vertex");
                const auto fragmentErrors = getShaderErrors(gl, fragment, "fragment");
                auto message = add("WebGLProgram: Shader Error ", call(gl, "getError"));
                message = add(message, " - ");
                message = add(message, "VALIDATE_STATUS ");
                const auto validate = callLater(gl, "getProgramParameter", [&] {
                    return Args{program, gl.get("VALIDATE_STATUS")};
                });
                message = add(message, validate);
                message = add(message, "\n\n");
                message = add(message, "Material Name: ");
                message = add(message, self.get("name"));
                message = add(message, "\n");
                message = add(message, "Material Type: ");
                message = add(message, self.get("type"));
                message = add(message, "\n\n");
                message = add(message, "Program Info Log: ");
                message = add(message, programLog);
                message = add(message, "\n");
                message = add(message, vertexErrors);
                message = add(message, "\n");
                message = add(message, fragmentErrors);
                ThreeUtils::error({message});
            }
        } else if (!programLog.strictEquals(""))
            ThreeUtils::warn({"WebGLProgram: Program Info Log:", programLog});
        else if (vertexLog.strictEquals("") || fragmentLog.strictEquals(""))
            haveDiagnostics = false;
        if (haveDiagnostics) {
            auto diagnostics = JSValue::object();
            literal(diagnostics, "runnable", runnable);
            literal(diagnostics, "programLog", programLog);
            auto v = JSValue::object();
            literal(v, "log", vertexLog);
            literal(v, "prefix", env.get("prefixVertex"));
            literal(diagnostics, "vertexShader", v);
            auto f = JSValue::object();
            literal(f, "log", fragmentLog);
            literal(f, "prefix", env.get("prefixFragment"));
            literal(diagnostics, "fragmentShader", f);
            auto receiver = self;
            receiver.set("diagnostics", diagnostics);
        }
    }
    call(gl, "deleteShader", {vertex});
    call(gl, "deleteShader", {fragment});
    const auto uniforms = env.get("module").get("WebGLUniforms").construct({gl, program});
    assign(env, "cachedUniforms", uniforms);
    const auto attributes = fetchAttributeLocations(gl, program);
    assign(env, "cachedAttributes", attributes);
}
JSValue constructProgram(const JSValue& self, const Args& args, const Args& captures) {
    const auto module = captures[0];
    const auto renderer = argument(args, 0), cacheKey = argument(args, 1),
               parameters = argument(args, 2), bindingStates = argument(args, 3);
    const auto gl = call(renderer, "getContext");
    const auto defines = parameters.get("defines");
    auto vertexShader = parameters.get("vertexShader");
    auto fragmentShader = parameters.get("fragmentShader");
    const auto shadowMapTypeDefine =
        fallback(module.get("shadowMapTypeDefines").get(parameters.get("shadowMapType")),
                 "SHADOWMAP_TYPE_BASIC");
    const JSValue envMapTypeDefine =
        parameters.get("envMapPMREM").strictEquals(true) ? "ENVMAP_TYPE_PMREM" : "ENVMAP_TYPE_CUBE";
    const auto envMapModeDefine =
        parameters.get("envMap").strictEquals(false)
            ? JSValue("ENVMAP_MODE_REFLECTION")
            : fallback(module.get("envMapModeDefines").get(parameters.get("envMapMode")),
                       "ENVMAP_MODE_REFLECTION");
    const auto envMapBlendingDefine =
        parameters.get("envMap").strictEquals(false)
            ? JSValue("ENVMAP_BLENDING_NONE")
            : fallback(module.get("envMapBlendingDefines").get(parameters.get("combine")),
                       "ENVMAP_BLENDING_NONE");
    const auto customVertexExtensions = generateVertexExtensions(module, parameters);
    const auto customDefines = generateDefines(defines);
    const auto program = call(gl, "createProgram");
    JSValue prefixVertex, prefixFragment;
    auto versionString = parameters.get("glslVersion").toBoolean()
                             ? add(add("#version ", parameters.get("glslVersion")), "\n")
                             : JSValue("");
    const auto prefix = [&](auto builder) {
        return builder(module, parameters, customDefines, customVertexExtensions,
                       shadowMapTypeDefine, envMapTypeDefine, envMapModeDefine,
                       envMapBlendingDefine, JSValue(), JSValue());
    };
    if (parameters.get("isRawShaderMaterial").toBoolean()) {
        prefixVertex = prefix(buildRawPrefix);
        if (prefixVertex.get("length").toNumber() > 0)
            prefixVertex = add(prefixVertex, "\n");
        prefixFragment = prefix(buildRawPrefix);
        if (prefixFragment.get("length").toNumber() > 0)
            prefixFragment = add(prefixFragment, "\n");
    } else {
        prefixVertex = prefix(buildVertexPrefix);
        prefixFragment = prefix(buildFragmentPrefix);
    }
    vertexShader = resolveIncludes(module, vertexShader);
    vertexShader = replaceLightNums(vertexShader, parameters);
    vertexShader = replaceClippingPlaneNums(vertexShader, parameters);
    fragmentShader = resolveIncludes(module, fragmentShader);
    fragmentShader = replaceLightNums(fragmentShader, parameters);
    fragmentShader = replaceClippingPlaneNums(fragmentShader, parameters);
    vertexShader = unrollLoops(module, vertexShader);
    fragmentShader = unrollLoops(module, fragmentShader);
    if (!parameters.get("isRawShaderMaterial").strictEquals(true)) {
        versionString = "#version 300 es\n";
        prefixVertex = buildVersionVertexPrefix(
            module, parameters, customDefines, customVertexExtensions, shadowMapTypeDefine,
            envMapTypeDefine, envMapModeDefine, envMapBlendingDefine, prefixVertex, prefixFragment);
        prefixFragment = buildVersionFragmentPrefix(
            module, parameters, customDefines, customVertexExtensions, shadowMapTypeDefine,
            envMapTypeDefine, envMapModeDefine, envMapBlendingDefine, prefixVertex, prefixFragment);
    }
    const auto vertexGlsl = add(add(versionString, prefixVertex), vertexShader);
    const auto fragmentGlsl = add(add(versionString, prefixFragment), fragmentShader);
    const auto shaderFunction = module.get("WebGLShader");
    const auto vertexType = gl.get("VERTEX_SHADER");
    const auto vertex = shaderFunction.call({}, {gl, vertexType, vertexGlsl});
    const auto fragmentType = gl.get("FRAGMENT_SHADER");
    const auto fragment = shaderFunction.call({}, {gl, fragmentType, fragmentGlsl});
    call(gl, "attachShader", {program, vertex});
    call(gl, "attachShader", {program, fragment});
    if (!parameters.get("index0AttributeName").isUndefined())
        callLater(gl, "bindAttribLocation",
                  [&] { return Args{program, 0, parameters.get("index0AttributeName")}; });
    else if (parameters.get("hasPositionAttribute").strictEquals(true))
        call(gl, "bindAttribLocation", {program, 0, "position"});
    call(gl, "linkProgram", {program});
    auto env = environment();
    bind(env, "module", module);
    bind(env, "renderer", renderer);
    bind(env, "gl", gl);
    bind(env, "program", program);
    bind(env, "glVertexShader", vertex);
    bind(env, "glFragmentShader", fragment);
    bind(env, "bindingStates", bindingStates);
    bind(env, "prefixVertex", prefixVertex);
    bind(env, "prefixFragment", prefixFragment);
    bind(env, "cachedUniforms", {});
    auto receiver = self;
    receiver.set("getUniforms", JSValue::constructor(
                                    [](const JSValue& self, const Args&, const Args& captures) {
                                        const auto env = captures[0];
                                        if (lexical(env, "cachedUniforms").isUndefined())
                                            onFirstUse(env, self);
                                        return lexical(env, "cachedUniforms");
                                    },
                                    {env}, "", 0, JSValue::ConstructorKind::Callable));
    bind(env, "cachedAttributes", {});
    receiver.set("getAttributes", JSValue::constructor(
                                      [](const JSValue& self, const Args&, const Args& captures) {
                                          const auto env = captures[0];
                                          if (lexical(env, "cachedAttributes").isUndefined())
                                              onFirstUse(env, self);
                                          return lexical(env, "cachedAttributes");
                                      },
                                      {env}, "", 0, JSValue::ConstructorKind::Callable));
    bind(env, "programReady",
         parameters.get("rendererExtensionParallelShaderCompile").strictEquals(false));
    receiver.set("isReady", JSValue::constructor(
                                [](const JSValue&, const Args&, const Args& captures) {
                                    const auto env = captures[0];
                                    if (lexical(env, "programReady").strictEquals(false)) {
                                        const auto ready =
                                            call(env.get("gl"), "getProgramParameter",
                                                 {env.get("program"), 0x91B1});
                                        assign(env, "programReady", ready);
                                    }
                                    return lexical(env, "programReady");
                                },
                                {env}, "", 0, JSValue::ConstructorKind::Callable));
    receiver.set("destroy", JSValue::constructor(
                                [](const JSValue& self, const Args&, const Args& captures) {
                                    const auto env = captures[0];
                                    call(env.get("bindingStates"), "releaseStatesOfProgram",
                                         {self});
                                    call(env.get("gl"), "deleteProgram", {env.get("program")});
                                    auto receiver = self;
                                    receiver.set("program", {});
                                    return JSValue();
                                },
                                {env}, "", 0, JSValue::ConstructorKind::Callable));
    receiver.set("type", parameters.get("shaderType"));
    receiver.set("name", parameters.get("shaderName"));
    const auto id = module.get("programIdCount");
    auto mutableModule = module;
    mutableModule.set("programIdCount", id.toNumber() + 1);
    receiver.set("id", id);
    receiver.set("cacheKey", cacheKey);
    receiver.set("usedTimes", 1);
    receiver.set("program", program);
    receiver.set("vertexShader", vertex);
    receiver.set("fragmentShader", fragment);
    return self;
}
JSValue matrix() {
    auto result = JSValue::object(nativeMatrix3Prototype());
    result.set("elements", array({1, 0, 0, 0, 1, 0, 0, 0, 1}));
    return result;
}
JSValue vector() {
    auto result = JSValue::object(nativeVector(std::make_shared<Vector3>()).getPrototype());
    result.set("x", 0);
    result.set("y", 0);
    result.set("z", 0);
    return result;
}
JSValue moduleObject() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope guard(initializing, "WebGLProgram");
    auto module = environment();
    bind(module, "WebGLUniforms", nativeWebGLUniformsConstructor());
    bind(module, "WebGLShader", nativeWebGLShaderConstructor());
    bind(module, "ShaderChunk", nativeShaderChunk());
    bind(module, "ColorManagement", nativeColorManagement());
    bind(module, "programIdCount", 0);
    bind(module, "m0", matrix());
    auto tones = JSValue::object();
    literal(tones, JSValue(LinearToneMapping).toString(), "Linear");
    literal(tones, JSValue(ReinhardToneMapping).toString(), "Reinhard");
    literal(tones, JSValue(CineonToneMapping).toString(), "Cineon");
    literal(tones, JSValue(ACESFilmicToneMapping).toString(), "ACESFilmic");
    literal(tones, JSValue(AgXToneMapping).toString(), "AgX");
    literal(tones, JSValue(NeutralToneMapping).toString(), "Neutral");
    literal(tones, JSValue(CustomToneMapping).toString(), "Custom");
    bind(module, "toneMappingFunctions", tones);
    bind(module, "v0", vector());
    bind(module, "filterEmptyLine",
         JSValue::constructor(
             [](const JSValue&, const Args& args, const Args&) {
                 return JSValue(!argument(args, 0).strictEquals(""));
             },
             {module}, "filterEmptyLine", 1, JSValue::ConstructorKind::Callable));
    bind(module, "includePattern", createNativeRegExp("^[ \\t]*#include +<([\\w\\d./]+)>", "gm"));
    bind(module, "includeReplacer",
         JSValue::constructor(
             [](const JSValue&, const Args& args, const Args& captures) {
                 const auto module = captures[0], include = argument(args, 1);
                 const auto string = module.get("ShaderChunk").get(include);
                 if (string.isUndefined()) {
                     const auto message =
                         add(add("THREE.WebGLProgram: Can not resolve #include <", include), ">");
                     throw std::runtime_error(message.toString());
                 }
                 return resolveIncludes(module, string);
             },
             {module}, "includeReplacer", 2, JSValue::ConstructorKind::Callable));
    bind(module, "unrollLoopPattern",
         createNativeRegExp("#pragma "
                            "unroll_loop_start\\s+for\\s*\\(\\s*int\\s+i\\s*=\\s*("
                            "\\d+)\\s*;\\s*i\\s*<\\s*(\\d+)\\s*;\\s*i\\s*\\+\\+"
                            "\\s*\\)\\s*{([\\s\\S]+?)}\\s+#pragma unroll_loop_end",
                            "g"));
    bind(module, "loopReplacer",
         JSValue::constructor(
             [](const JSValue&, const Args& args, const Args&) {
                 const auto start = argument(args, 1), end = argument(args, 2),
                            snippet = argument(args, 3);
                 JSValue string = "";
                 for (double i = parseInt(start); i < parseInt(end); ++i) {
                     const auto first = callLater(snippet, "replace", [&] {
                         const auto pattern = createNativeRegExp("\\[\\s*i\\s*\\]", "g");
                         return Args{pattern, add(add("[ ", i), " ]")};
                     });
                     const auto next = callLater(first, "replace", [&] {
                         return Args{createNativeRegExp("UNROLLED_LOOP_INDEX", "g"), i};
                     });
                     string = add(string, next);
                 }
                 return string;
             },
             {module}, "loopReplacer", 4, JSValue::ConstructorKind::Callable));
    auto shadows = JSValue::object();
    literal(shadows, JSValue(PCFShadowMap).toString(), "SHADOWMAP_TYPE_PCF");
    literal(shadows, JSValue(VSMShadowMap).toString(), "SHADOWMAP_TYPE_VSM");
    bind(module, "shadowMapTypeDefines", shadows);
    auto modes = JSValue::object();
    literal(modes, JSValue(CubeRefractionMapping).toString(), "ENVMAP_MODE_REFRACTION");
    bind(module, "envMapModeDefines", modes);
    auto blends = JSValue::object();
    literal(blends, JSValue(MultiplyOperation).toString(), "ENVMAP_BLENDING_MULTIPLY");
    literal(blends, JSValue(MixOperation).toString(), "ENVMAP_BLENDING_MIX");
    literal(blends, JSValue(AddOperation).toString(), "ENVMAP_BLENDING_ADD");
    bind(module, "envMapBlendingDefines", blends);
    bind(module, "WebGLProgram",
         JSValue::constructor(constructProgram, {module}, "WebGLProgram", 4,
                              JSValue::ConstructorKind::Callable));
    module.retainForHeapLifetime();
    cached = JSWeakValue(module);
    return module;
}
} // namespace
JSValue nativeWebGLProgramConstructor() {
    return moduleObject().get("WebGLProgram");
}
WebGLProgram::WebGLProgram(const JSValue& renderer, const JSValue& cacheKey,
                           const JSValue& parameters, const JSValue& bindingStates) {
    static_cast<void>(JSWeakValue(renderer));
    static_cast<void>(JSWeakValue(cacheKey));
    static_cast<void>(JSWeakValue(parameters));
    static_cast<void>(JSWeakValue(bindingStates));
    object_ =
        nativeWebGLProgramConstructor().construct({renderer, cacheKey, parameters, bindingStates});
}
JSValue WebGLProgram::getUniforms() {
    const auto value = object();
    return call(value, "getUniforms");
}
JSValue WebGLProgram::getAttributes() {
    const auto value = object();
    return call(value, "getAttributes");
}
JSValue WebGLProgram::isReady() {
    const auto value = object();
    return call(value, "isReady");
}
JSValue WebGLProgram::destroy() {
    const auto value = object();
    return call(value, "destroy");
}

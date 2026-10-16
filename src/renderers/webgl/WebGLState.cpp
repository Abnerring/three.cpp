// Direct port of three.js WebGLState.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLState.h"
#include "renderers/webgl/WebGLStateSource.inc"
#include "constants.h"
#include "core/JSModuleObject.h"
#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include "core/JSWeakMap.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Vector4.h"
#include "utils.h"
#include <cmath>
#include <limits>
#include <tuple>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
void literal(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
template<class F> JSValue later(const JSValue& receiver, const char* name, F&& arguments) {
    const auto method = receiver.get(name);
    const auto args = arguments();
    return method.call(receiver, args);
}
JSValue call(const JSValue& receiver, const char* name, const Args& arguments = {}) {
    return receiver.get(name).call(receiver, arguments);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber(); return x + b.toNumber();
}
double subtract(const JSValue& left, const JSValue& right) {
    const auto a = left.toNumber(); return a - right.toNumber();
}
double multiply(const JSValue& left, const JSValue& right) {
    const auto a = left.toNumber(); return a * right.toNumber();
}
JSValue vector(double x = 0, double y = 0, double z = 0, double w = 1) {
    const auto prototype = nativeVector(std::make_shared<Vector4>()).getPrototype();
    auto result = JSValue::object(prototype);
    result.set("x", x); result.set("y", y); result.set("z", z); result.set("w", w);
    return result;
}
JSValue color() {
    auto result = JSValue::object(nativeColorPrototype());
    result.set("isColor", true); result.set("r", 1); result.set("g", 1); result.set("b", 1);
    return call(result, "set", {0, 0, 0});
}
JSValue gl(const JSValue& state) { return state.get("gl"); }
void enable(JSValue state, const JSValue& id) {
    if (!state.get("enabledCapabilities").get(id).strictEquals(true)) {
        call(gl(state), "enable", {id});
        state.get("enabledCapabilities").set(id, true);
    }
}
void disable(JSValue state, const JSValue& id) {
    if (!state.get("enabledCapabilities").get(id).strictEquals(false)) {
        call(gl(state), "disable", {id});
        state.get("enabledCapabilities").set(id, false);
    }
}
JSValue bufferMethod(const JSValue& self, const Args& args, const Args& captures) {
    auto state = captures[0], buffer = captures[1];
    const auto kind = captures[2].string(), name = captures[3].string();
    if (name == "setLocked") { buffer.set("locked", arg(args, 0)); return {}; }
    if (name == "reset") {
        buffer.set("locked", false);
        if (kind == "color") {
            buffer.set("currentColorMask", nullptr);
            call(buffer.get("currentColorClear"), "set", {-1, 0, 0, 0});
        } else if (kind == "depth") {
            for (const auto* field : {"currentDepthMask", "currentDepthFunc", "currentDepthClear"}) buffer.set(field, nullptr);
            buffer.set("currentReversed", false);
        } else {
            for (const auto* field : {"currentStencilMask", "currentStencilFunc", "currentStencilRef", "currentStencilFuncMask",
                                     "currentStencilFail", "currentStencilZFail", "currentStencilZPass", "currentStencilClear"})
                buffer.set(field, nullptr);
        }
        return {};
    }
    if (kind == "color") {
        if (name == "setMask") {
            const auto mask = arg(args, 0);
            if (!buffer.get("currentColorMask").strictEquals(mask) && !buffer.get("locked").toBoolean()) {
                call(gl(state), "colorMask", {mask, mask, mask, mask}); buffer.set("currentColorMask", mask);
            }
        } else if (name == "setClear") {
            auto r = arg(args, 0), g = arg(args, 1), b = arg(args, 2), a = arg(args, 3);
            if (arg(args, 4).strictEquals(true)) { r = multiply(r, a); g = multiply(g, a); b = multiply(b, a); }
            call(buffer.get("color"), "set", {r, g, b, a});
            if (later(buffer.get("currentColorClear"), "equals", [&] { return Args{buffer.get("color")}; }).strictEquals(false)) {
                call(gl(state), "clearColor", {r, g, b, a});
                later(buffer.get("currentColorClear"), "copy", [&] { return Args{buffer.get("color")}; });
            }
        }
    } else if (kind == "depth") {
        if (name == "setReversed") {
            const auto reversed = arg(args, 0);
            if (!buffer.get("currentReversed").strictEquals(reversed)) {
                const auto ext = call(state.get("extensions"), "get", {"EXT_clip_control"});
                later(ext, "clipControlEXT", [&] {
                    const auto origin = ext.get("LOWER_LEFT_EXT");
                    const auto depth = ext.get(reversed.toBoolean() ? "ZERO_TO_ONE_EXT" : "NEGATIVE_ONE_TO_ONE_EXT");
                    return Args{origin, depth};
                });
                buffer.set("currentReversed", reversed);
                const auto oldDepth = buffer.get("currentDepthClear"); buffer.set("currentDepthClear", nullptr);
                call(self, "setClear", {oldDepth});
            }
        } else if (name == "getReversed") return buffer.get("currentReversed");
        else if (name == "setTest") {
            const auto id = gl(state).get("DEPTH_TEST");
            if (arg(args, 0).toBoolean()) enable(state, id); else disable(state, id);
        } else if (name == "setMask") {
            const auto mask = arg(args, 0);
            if (!buffer.get("currentDepthMask").strictEquals(mask) && !buffer.get("locked").toBoolean()) {
                call(gl(state), "depthMask", {mask}); buffer.set("currentDepthMask", mask);
            }
        } else if (name == "setFunc") {
            auto func = arg(args, 0);
            if (buffer.get("currentReversed").toBoolean()) func = state.get("reversedDepthFunctions").get(func);
            if (!buffer.get("currentDepthFunc").strictEquals(func)) {
                const char* mapped = func.strictEquals(NeverDepth) ? "NEVER" : func.strictEquals(AlwaysDepth) ? "ALWAYS" :
                    func.strictEquals(LessDepth) ? "LESS" : func.strictEquals(LessEqualDepth) ? "LEQUAL" :
                    func.strictEquals(EqualDepth) ? "EQUAL" : func.strictEquals(GreaterEqualDepth) ? "GEQUAL" :
                    func.strictEquals(GreaterDepth) ? "GREATER" : func.strictEquals(NotEqualDepth) ? "NOTEQUAL" : "LEQUAL";
                later(gl(state), "depthFunc", [&] { return Args{gl(state).get(mapped)}; });
                buffer.set("currentDepthFunc", func);
            }
        } else if (name == "setClear") {
            auto depth = arg(args, 0);
            if (!buffer.get("currentDepthClear").strictEquals(depth)) {
                buffer.set("currentDepthClear", depth);
                if (buffer.get("currentReversed").toBoolean()) depth = subtract(1, depth);
                call(gl(state), "clearDepth", {depth});
            }
        }
    } else {
        if (name == "setTest") {
            if (!buffer.get("locked").toBoolean()) {
                const auto id = gl(state).get("STENCIL_TEST");
                if (arg(args, 0).toBoolean()) enable(state, id); else disable(state, id);
            }
        } else if (name == "setMask") {
            const auto mask = arg(args, 0);
            if (!buffer.get("currentStencilMask").strictEquals(mask) && !buffer.get("locked").toBoolean()) {
                call(gl(state), "stencilMask", {mask}); buffer.set("currentStencilMask", mask);
            }
        } else if (name == "setFunc") {
            const auto func = arg(args, 0), ref = arg(args, 1), mask = arg(args, 2);
            if (!buffer.get("currentStencilFunc").strictEquals(func) || !buffer.get("currentStencilRef").strictEquals(ref) ||
                !buffer.get("currentStencilFuncMask").strictEquals(mask)) {
                call(gl(state), "stencilFunc", {func, ref, mask});
                buffer.set("currentStencilFunc", func); buffer.set("currentStencilRef", ref); buffer.set("currentStencilFuncMask", mask);
            }
        } else if (name == "setOp") {
            const auto fail = arg(args, 0), zfail = arg(args, 1), zpass = arg(args, 2);
            if (!buffer.get("currentStencilFail").strictEquals(fail) || !buffer.get("currentStencilZFail").strictEquals(zfail) ||
                !buffer.get("currentStencilZPass").strictEquals(zpass)) {
                call(gl(state), "stencilOp", {fail, zfail, zpass});
                buffer.set("currentStencilFail", fail); buffer.set("currentStencilZFail", zfail); buffer.set("currentStencilZPass", zpass);
            }
        } else if (name == "setClear") {
            const auto stencil = arg(args, 0);
            if (!buffer.get("currentStencilClear").strictEquals(stencil)) {
                call(gl(state), "clearStencil", {stencil}); buffer.set("currentStencilClear", stencil);
            }
        }
    }
    return {};
}
struct Method { const char* name; unsigned arity; const char* source; };
JSValue makeBuffer(JSValue state, const char* kind) {
    auto frame = JSValue::object(nullptr), result = JSValue::object(); literal(frame, "locked", false);
    std::vector<Method> methods;
    if (std::string(kind) == "color") {
        literal(frame, "color", vector()); literal(frame, "currentColorMask", nullptr);
        literal(frame, "currentColorClear", vector(0, 0, 0, 0));
        methods = {{"setMask", 1, WebGLStateSource::ColorBuffer_setMask}, {"setLocked", 1, WebGLStateSource::ColorBuffer_setLocked},
                   {"setClear", 5, WebGLStateSource::ColorBuffer_setClear}, {"reset", 0, WebGLStateSource::ColorBuffer_reset}};
    } else if (std::string(kind) == "depth") {
        literal(frame, "currentReversed", false);
        for (const auto* name : {"currentDepthMask", "currentDepthFunc", "currentDepthClear"}) literal(frame, name, nullptr);
        methods = {{"setReversed", 1, WebGLStateSource::DepthBuffer_setReversed}, {"getReversed", 0, WebGLStateSource::DepthBuffer_getReversed},
                   {"setTest", 1, WebGLStateSource::DepthBuffer_setTest}, {"setMask", 1, WebGLStateSource::DepthBuffer_setMask},
                   {"setFunc", 1, WebGLStateSource::DepthBuffer_setFunc}, {"setLocked", 1, WebGLStateSource::DepthBuffer_setLocked},
                   {"setClear", 1, WebGLStateSource::DepthBuffer_setClear}, {"reset", 0, WebGLStateSource::DepthBuffer_reset}};
    } else {
        for (const auto* name : {"currentStencilMask", "currentStencilFunc", "currentStencilRef", "currentStencilFuncMask",
                                  "currentStencilFail", "currentStencilZFail", "currentStencilZPass", "currentStencilClear"}) literal(frame, name, nullptr);
        methods = {{"setTest", 1, WebGLStateSource::StencilBuffer_setTest}, {"setMask", 1, WebGLStateSource::StencilBuffer_setMask},
                   {"setFunc", 3, WebGLStateSource::StencilBuffer_setFunc}, {"setOp", 3, WebGLStateSource::StencilBuffer_setOp},
                   {"setLocked", 1, WebGLStateSource::StencilBuffer_setLocked}, {"setClear", 1, WebGLStateSource::StencilBuffer_setClear},
                   {"reset", 0, WebGLStateSource::StencilBuffer_reset}};
    }
    for (const auto& method : methods) {
        auto fn = JSValue::constructor(bufferMethod, {state, frame, kind, method.name}, method.name, method.arity, JSValue::ConstructorKind::Callable);
        fn.setFunctionSourceText(method.source); literal(result, method.name, fn);
    }
    return result;
}
void flipSided(JSValue state, const JSValue& flip) {
    if (!state.get("currentFlipSided").strictEquals(flip)) {
        later(gl(state), "frontFace", [&] { return Args{gl(state).get(flip.toBoolean() ? "CW" : "CCW")}; });
        state.set("currentFlipSided", flip);
    }
}
void cullFace(JSValue state, const JSValue& face) {
    if (!face.strictEquals(CullFaceNone)) {
        enable(state, gl(state).get("CULL_FACE"));
        if (!face.strictEquals(state.get("currentCullFace"))) {
            later(gl(state), "cullFace", [&] { return Args{gl(state).get(face.strictEquals(CullFaceBack) ? "BACK" :
                face.strictEquals(CullFaceFront) ? "FRONT" : "FRONT_AND_BACK")}; });
        }
    } else disable(state, gl(state).get("CULL_FACE"));
    state.set("currentCullFace", face);
}
void polygonOffset(JSValue state, const JSValue& enabled, JSValue factor, JSValue units) {
    if (enabled.toBoolean()) {
        enable(state, gl(state).get("POLYGON_OFFSET_FILL"));
        if (!state.get("currentPolygonOffsetFactor").strictEquals(factor) || !state.get("currentPolygonOffsetUnits").strictEquals(units)) {
            state.set("currentPolygonOffsetFactor", factor); state.set("currentPolygonOffsetUnits", units);
            if (call(state.get("depthBuffer"), "getReversed").toBoolean()) { factor = -factor.toNumber(); units = -units.toNumber(); }
            call(gl(state), "polygonOffset", {factor, units});
        }
    } else disable(state, gl(state).get("POLYGON_OFFSET_FILL"));
}
void blending(JSValue state, const Args& args) {
    const auto blend = arg(args, 0), premultiplied = arg(args, 9);
    if (blend.strictEquals(NoBlending)) {
        if (state.get("currentBlendingEnabled").strictEquals(true)) {
            disable(state, gl(state).get("BLEND")); state.set("currentBlendingEnabled", false);
        }
        return;
    }
    if (state.get("currentBlendingEnabled").strictEquals(false)) {
        enable(state, gl(state).get("BLEND")); state.set("currentBlendingEnabled", true);
    }
    if (!blend.strictEquals(CustomBlending)) {
        if (!blend.strictEquals(state.get("currentBlending")) || !premultiplied.strictEquals(state.get("currentPremultipledAlpha"))) {
            if (!state.get("currentBlendEquation").strictEquals(AddEquation) || !state.get("currentBlendEquationAlpha").strictEquals(AddEquation)) {
                later(gl(state), "blendEquation", [&] { return Args{gl(state).get("FUNC_ADD")}; });
                state.set("currentBlendEquation", AddEquation); state.set("currentBlendEquationAlpha", AddEquation);
            }
            const char* method = nullptr; std::vector<const char*> fields;
            if (premultiplied.toBoolean()) {
                if (blend.strictEquals(NormalBlending)) { method = "blendFuncSeparate"; fields = {"ONE", "ONE_MINUS_SRC_ALPHA", "ONE", "ONE_MINUS_SRC_ALPHA"}; }
                else if (blend.strictEquals(AdditiveBlending)) { method = "blendFunc"; fields = {"ONE", "ONE"}; }
                else if (blend.strictEquals(SubtractiveBlending)) { method = "blendFuncSeparate"; fields = {"ZERO", "ONE_MINUS_SRC_COLOR", "ZERO", "ONE"}; }
                else if (blend.strictEquals(MultiplyBlending)) { method = "blendFuncSeparate"; fields = {"DST_COLOR", "ONE_MINUS_SRC_ALPHA", "ZERO", "ONE"}; }
                else ThreeUtils::error({"WebGLState: Invalid blending: ", blend});
            } else {
                if (blend.strictEquals(NormalBlending)) { method = "blendFuncSeparate"; fields = {"SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "ONE", "ONE_MINUS_SRC_ALPHA"}; }
                else if (blend.strictEquals(AdditiveBlending)) { method = "blendFuncSeparate"; fields = {"SRC_ALPHA", "ONE", "ONE", "ONE"}; }
                else if (blend.strictEquals(SubtractiveBlending)) ThreeUtils::error({"WebGLState: SubtractiveBlending requires material.premultipliedAlpha = true"});
                else if (blend.strictEquals(MultiplyBlending)) ThreeUtils::error({"WebGLState: MultiplyBlending requires material.premultipliedAlpha = true"});
                else ThreeUtils::error({"WebGLState: Invalid blending: ", blend});
            }
            if (method) later(gl(state), method, [&] { Args out; for (const auto* name : fields) out.push_back(gl(state).get(name)); return out; });
            for (const auto* name : {"currentBlendSrc", "currentBlendDst", "currentBlendSrcAlpha", "currentBlendDstAlpha"}) state.set(name, nullptr);
            call(state.get("currentBlendColor"), "set", {0, 0, 0}); state.set("currentBlendAlpha", 0);
            state.set("currentBlending", blend); state.set("currentPremultipledAlpha", premultiplied);
        }
        return;
    }
    const auto equation = arg(args, 1), src = arg(args, 2), dst = arg(args, 3), blendColor = arg(args, 7), alpha = arg(args, 8);
    auto equationAlpha = arg(args, 4), srcAlpha = arg(args, 5), dstAlpha = arg(args, 6);
    if (!equationAlpha.toBoolean()) equationAlpha = equation;
    if (!srcAlpha.toBoolean()) srcAlpha = src;
    if (!dstAlpha.toBoolean()) dstAlpha = dst;
    if (!equation.strictEquals(state.get("currentBlendEquation")) || !equationAlpha.strictEquals(state.get("currentBlendEquationAlpha"))) {
        later(gl(state), "blendEquationSeparate", [&] {
            const auto a = state.get("equationToGL").get(equation), b = state.get("equationToGL").get(equationAlpha);
            return Args{a, b};
        });
        state.set("currentBlendEquation", equation); state.set("currentBlendEquationAlpha", equationAlpha);
    }
    if (!src.strictEquals(state.get("currentBlendSrc")) || !dst.strictEquals(state.get("currentBlendDst")) ||
        !srcAlpha.strictEquals(state.get("currentBlendSrcAlpha")) || !dstAlpha.strictEquals(state.get("currentBlendDstAlpha"))) {
        later(gl(state), "blendFuncSeparate", [&] {
            const auto a = state.get("factorToGL").get(src), b = state.get("factorToGL").get(dst),
                       c = state.get("factorToGL").get(srcAlpha), d = state.get("factorToGL").get(dstAlpha);
            return Args{a, b, c, d};
        });
        state.set("currentBlendSrc", src); state.set("currentBlendDst", dst);
        state.set("currentBlendSrcAlpha", srcAlpha); state.set("currentBlendDstAlpha", dstAlpha);
    }
    if (later(blendColor, "equals", [&] { return Args{state.get("currentBlendColor")}; }).strictEquals(false) ||
        !alpha.strictEquals(state.get("currentBlendAlpha"))) {
        later(gl(state), "blendColor", [&] {
            const auto r = blendColor.get("r"), g = blendColor.get("g"), b = blendColor.get("b"); return Args{r, g, b, alpha};
        });
        call(state.get("currentBlendColor"), "copy", {blendColor}); state.set("currentBlendAlpha", alpha);
    }
    state.set("currentBlending", blend); state.set("currentPremultipledAlpha", false);
}
void material(JSValue state, const JSValue& value, const JSValue& frontFaceCW) {
    if (value.get("side").strictEquals(DoubleSide)) disable(state, gl(state).get("CULL_FACE"));
    else enable(state, gl(state).get("CULL_FACE"));
    bool flip = value.get("side").strictEquals(BackSide); if (frontFaceCW.toBoolean()) flip = !flip; flipSided(state, flip);
    if (value.get("blending").strictEquals(NormalBlending) && value.get("transparent").strictEquals(false)) blending(state, {NoBlending});
    else {
        Args args; for (const auto* name : {"blending", "blendEquation", "blendSrc", "blendDst", "blendEquationAlpha", "blendSrcAlpha",
                                            "blendDstAlpha", "blendColor", "blendAlpha", "premultipliedAlpha"}) args.push_back(value.get(name));
        blending(state, args);
    }
    later(state.get("depthBuffer"), "setFunc", [&] { return Args{value.get("depthFunc")}; });
    later(state.get("depthBuffer"), "setTest", [&] { return Args{value.get("depthTest")}; });
    later(state.get("depthBuffer"), "setMask", [&] { return Args{value.get("depthWrite")}; });
    later(state.get("colorBuffer"), "setMask", [&] { return Args{value.get("colorWrite")}; });
    const auto stencilWrite = value.get("stencilWrite"); call(state.get("stencilBuffer"), "setTest", {stencilWrite});
    if (stencilWrite.toBoolean()) {
        later(state.get("stencilBuffer"), "setMask", [&] { return Args{value.get("stencilWriteMask")}; });
        later(state.get("stencilBuffer"), "setFunc", [&] {
            const auto a = value.get("stencilFunc"), b = value.get("stencilRef"), c = value.get("stencilFuncMask"); return Args{a, b, c};
        });
        later(state.get("stencilBuffer"), "setOp", [&] {
            const auto a = value.get("stencilFail"), b = value.get("stencilZFail"), c = value.get("stencilZPass"); return Args{a, b, c};
        });
    }
    const auto offset = value.get("polygonOffset"), factor = value.get("polygonOffsetFactor"), units = value.get("polygonOffsetUnits");
    polygonOffset(state, offset, factor, units);
    if (value.get("alphaToCoverage").strictEquals(true)) enable(state, gl(state).get("SAMPLE_ALPHA_TO_COVERAGE"));
    else disable(state, gl(state).get("SAMPLE_ALPHA_TO_COVERAGE"));
}
JSValue createTexture(JSValue state, const JSValue& type, const JSValue& target, double count, const JSValue& dimensions = {}) {
    const auto data = NativeArray_detail::wrap(InterpolantArray(Uint8Array(4)));
    const auto texture = call(gl(state), "createTexture");
    call(gl(state), "bindTexture", {type, texture});
    later(gl(state), "texParameteri", [&] {
        const auto filter = gl(state).get("TEXTURE_MIN_FILTER"), nearest = gl(state).get("NEAREST"); return Args{type, filter, nearest};
    });
    later(gl(state), "texParameteri", [&] {
        const auto filter = gl(state).get("TEXTURE_MAG_FILTER"), nearest = gl(state).get("NEAREST"); return Args{type, filter, nearest};
    });
    for (double i = 0; i < count; ++i) {
        if (type.strictEquals(gl(state).get("TEXTURE_3D")) || type.strictEquals(gl(state).get("TEXTURE_2D_ARRAY"))) {
            later(gl(state), "texImage3D", [&] {
                const auto internal = gl(state).get("RGBA"), format = gl(state).get("RGBA"), bytes = gl(state).get("UNSIGNED_BYTE");
                return Args{target, 0, internal, 1, 1, dimensions, 0, format, bytes, data};
            });
        } else {
            later(gl(state), "texImage2D", [&] {
                const auto face = add(target, i), internal = gl(state).get("RGBA"), format = gl(state).get("RGBA"), bytes = gl(state).get("UNSIGNED_BYTE");
                return Args{face, 0, internal, 1, 1, 0, format, bytes, data};
            });
        }
    }
    return texture;
}
JSValue defaultSlot(JSValue state) { return subtract(add(gl(state).get("TEXTURE0"), state.get("maxTextures")), 1); }
void reset(JSValue state) {
    for (const auto* name : {"BLEND", "CULL_FACE", "DEPTH_TEST", "POLYGON_OFFSET_FILL", "SCISSOR_TEST", "STENCIL_TEST", "SAMPLE_ALPHA_TO_COVERAGE"})
        later(gl(state), "disable", [&] { return Args{gl(state).get(name)}; });
    later(gl(state), "blendEquation", [&] { return Args{gl(state).get("FUNC_ADD")}; });
    later(gl(state), "blendFunc", [&] { const auto a = gl(state).get("ONE"), b = gl(state).get("ZERO"); return Args{a, b}; });
    later(gl(state), "blendFuncSeparate", [&] {
        const auto a = gl(state).get("ONE"), b = gl(state).get("ZERO"), c = gl(state).get("ONE"), d = gl(state).get("ZERO"); return Args{a, b, c, d};
    });
    call(gl(state), "blendColor", {0, 0, 0, 0}); call(gl(state), "colorMask", {true, true, true, true});
    call(gl(state), "clearColor", {0, 0, 0, 0}); call(gl(state), "depthMask", {true});
    later(gl(state), "depthFunc", [&] { return Args{gl(state).get("LESS")}; });
    call(state.get("depthBuffer"), "setReversed", {false}); call(gl(state), "clearDepth", {1});
    call(gl(state), "stencilMask", {4294967295.0});
    later(gl(state), "stencilFunc", [&] { return Args{gl(state).get("ALWAYS"), 0, 4294967295.0}; });
    later(gl(state), "stencilOp", [&] {
        const auto a = gl(state).get("KEEP"), b = gl(state).get("KEEP"), c = gl(state).get("KEEP"); return Args{a, b, c};
    });
    call(gl(state), "clearStencil", {0});
    later(gl(state), "cullFace", [&] { return Args{gl(state).get("BACK")}; });
    later(gl(state), "frontFace", [&] { return Args{gl(state).get("CCW")}; });
    call(gl(state), "polygonOffset", {0, 0}); later(gl(state), "activeTexture", [&] { return Args{gl(state).get("TEXTURE0")}; });
    for (const auto* name : {"FRAMEBUFFER", "DRAW_FRAMEBUFFER", "READ_FRAMEBUFFER"})
        later(gl(state), "bindFramebuffer", [&] { return Args{gl(state).get(name), nullptr}; });
    call(gl(state), "useProgram", {nullptr}); call(gl(state), "lineWidth", {1});
    for (const auto* name : {"scissor", "viewport"}) later(gl(state), name, [&] {
        const auto width = gl(state).get("canvas").get("width"), height = gl(state).get("canvas").get("height"); return Args{0, 0, width, height};
    });
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, JSValue>>{
             {"PACK_ALIGNMENT", 4}, {"UNPACK_ALIGNMENT", 4}, {"UNPACK_FLIP_Y_WEBGL", false}, {"UNPACK_PREMULTIPLY_ALPHA_WEBGL", false}})
        later(gl(state), "pixelStorei", [&] { return Args{gl(state).get(name), value}; });
    later(gl(state), "pixelStorei", [&] {
        const auto a = gl(state).get("UNPACK_COLORSPACE_CONVERSION_WEBGL"), b = gl(state).get("BROWSER_DEFAULT_WEBGL"); return Args{a, b};
    });
    for (const auto* name : {"PACK_ROW_LENGTH", "PACK_SKIP_PIXELS", "PACK_SKIP_ROWS", "UNPACK_ROW_LENGTH", "UNPACK_IMAGE_HEIGHT",
                             "UNPACK_SKIP_PIXELS", "UNPACK_SKIP_ROWS", "UNPACK_SKIP_IMAGES"})
        later(gl(state), "pixelStorei", [&] { return Args{gl(state).get(name), 0}; });
    for (const auto* name : {"enabledCapabilities", "parameters", "currentBoundTextures", "currentBoundFramebuffers", "uboBindings"}) state.set(name, JSValue::object());
    state.set("currentTextureSlot", nullptr); state.set("currentDrawbuffers", createNativeWeakMap()); state.set("defaultDrawbuffers", JSValue::array());
    state.set("currentProgram", nullptr); state.set("currentBlendingEnabled", false);
    for (const auto* name : {"currentBlending", "currentBlendEquation", "currentBlendSrc", "currentBlendDst", "currentBlendEquationAlpha",
                             "currentBlendSrcAlpha", "currentBlendDstAlpha", "currentFlipSided", "currentCullFace", "currentLineWidth",
                             "currentPolygonOffsetFactor", "currentPolygonOffsetUnits"}) state.set(name, nullptr);
    state.set("currentBlendColor", color()); state.set("currentBlendAlpha", 0); state.set("currentPremultipledAlpha", false);
    for (const auto* name : {"currentScissor", "currentViewport"}) later(state.get(name), "set", [&] {
        const auto width = gl(state).get("canvas").get("width"), height = gl(state).get("canvas").get("height"); return Args{0, 0, width, height};
    });
    call(state.get("colorBuffer"), "reset"); call(state.get("depthBuffer"), "reset"); call(state.get("stencilBuffer"), "reset");
}
JSValue exceptionValue(const std::exception& error, const char* name) {
    auto value = JSValue::object();
    JSPropertyDescriptor d; d.value = name; d.writable = d.configurable = true; value.defineProperty("name", d);
    d.value = error.what(); value.defineProperty("message", d);
    return value;
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    auto state = captures[0]; const auto name = captures[1].string();
    if (name == "enable") enable(state, arg(args, 0));
    else if (name == "disable") disable(state, arg(args, 0));
    else if (name == "bindFramebuffer") {
        const auto target = arg(args, 0), framebuffer = arg(args, 1);
        if (!state.get("currentBoundFramebuffers").get(target).strictEquals(framebuffer)) {
            call(gl(state), "bindFramebuffer", {target, framebuffer}); state.get("currentBoundFramebuffers").set(target, framebuffer);
            if (target.strictEquals(gl(state).get("DRAW_FRAMEBUFFER"))) {
                const auto cache = state.get("currentBoundFramebuffers"), key = gl(state).get("FRAMEBUFFER"); auto writable = cache; writable.set(key, framebuffer);
            }
            if (target.strictEquals(gl(state).get("FRAMEBUFFER"))) {
                const auto cache = state.get("currentBoundFramebuffers"), key = gl(state).get("DRAW_FRAMEBUFFER"); auto writable = cache; writable.set(key, framebuffer);
            }
            return true;
        }
        return false;
    } else if (name == "drawBuffers") {
        const auto target = arg(args, 0), framebuffer = arg(args, 1); auto buffers = state.get("defaultDrawbuffers"); bool update = false;
        if (target.toBoolean()) {
            buffers = call(state.get("currentDrawbuffers"), "get", {framebuffer});
            if (buffers.isUndefined()) { buffers = JSValue::array(); call(state.get("currentDrawbuffers"), "set", {framebuffer, buffers}); }
            const auto textures = target.get("textures");
            if (!buffers.get("length").strictEquals(textures.get("length")) || !buffers.get("0").strictEquals(gl(state).get("COLOR_ATTACHMENT0"))) {
                const auto length = textures.get("length");
                for (double i = 0; i < length.toNumber(); ++i) buffers.set(JSValue(i), add(gl(state).get("COLOR_ATTACHMENT0"), i));
                buffers.set("length", textures.get("length")); update = true;
            }
        } else if (!buffers.get("0").strictEquals(gl(state).get("BACK"))) { buffers.set("0", gl(state).get("BACK")); update = true; }
        if (update) call(gl(state), "drawBuffers", {buffers});
    } else if (name == "useProgram") {
        const auto program = arg(args, 0);
        if (!state.get("currentProgram").strictEquals(program)) { call(gl(state), "useProgram", {program}); state.set("currentProgram", program); return true; }
        return false;
    } else if (name == "setBlending") blending(state, args);
    else if (name == "setMaterial") material(state, arg(args, 0), arg(args, 1));
    else if (name == "setFlipSided") flipSided(state, arg(args, 0));
    else if (name == "setCullFace") cullFace(state, arg(args, 0));
    else if (name == "setLineWidth") {
        const auto width = arg(args, 0);
        if (!width.strictEquals(state.get("currentLineWidth"))) {
            if (state.get("lineWidthAvailable").toBoolean()) call(gl(state), "lineWidth", {width}); state.set("currentLineWidth", width);
        }
    } else if (name == "setPolygonOffset") polygonOffset(state, arg(args, 0), arg(args, 1), arg(args, 2));
    else if (name == "setScissorTest") {
        const auto id = gl(state).get("SCISSOR_TEST"); if (arg(args, 0).toBoolean()) enable(state, id); else disable(state, id);
    } else if (name == "activeTexture") {
        auto slot = arg(args, 0); if (slot.isUndefined()) slot = defaultSlot(state);
        if (!state.get("currentTextureSlot").strictEquals(slot)) { call(gl(state), "activeTexture", {slot}); state.set("currentTextureSlot", slot); }
    } else if (name == "bindTexture") {
        const auto type = arg(args, 0), texture = arg(args, 1); auto slot = arg(args, 2);
        if (slot.isUndefined()) slot = state.get("currentTextureSlot").isNull() ? defaultSlot(state) : state.get("currentTextureSlot");
        auto bound = state.get("currentBoundTextures").get(slot);
        if (bound.isUndefined()) {
            bound = JSValue::object(); literal(bound, "type", {}); literal(bound, "texture", {});
            state.get("currentBoundTextures").set(slot, bound);
        }
        if (!bound.get("type").strictEquals(type) || !bound.get("texture").strictEquals(texture)) {
            if (!state.get("currentTextureSlot").strictEquals(slot)) { call(gl(state), "activeTexture", {slot}); state.set("currentTextureSlot", slot); }
            later(gl(state), "bindTexture", [&] { return Args{type, texture.toBoolean() ? texture : state.get("emptyTextures").get(type)}; });
            bound.set("type", type); bound.set("texture", texture);
        }
    } else if (name == "unbindTexture") {
        const auto cache = state.get("currentBoundTextures"), slot = state.get("currentTextureSlot"); auto bound = cache.get(slot);
        if (!bound.isUndefined() && !bound.get("type").isUndefined()) {
            later(gl(state), "bindTexture", [&] { return Args{bound.get("type"), nullptr}; }); bound.set("type", {}); bound.set("texture", {});
        }
    } else if (name == "getParameter") {
        const auto parameter = arg(args, 0);
        if (!state.get("parameters").get(parameter).isUndefined()) return state.get("parameters").get(parameter);
        return call(gl(state), "getParameter", {parameter});
    } else if (name == "pixelStorei") {
        const auto parameter = arg(args, 0), value = arg(args, 1);
        if (!state.get("parameters").get(parameter).strictEquals(value)) { call(gl(state), "pixelStorei", {parameter, value}); state.get("parameters").set(parameter, value); }
    } else if (name == "scissor" || name == "viewport") {
        const auto value = arg(args, 0), cached = state.get(name == "scissor" ? "currentScissor" : "currentViewport");
        if (call(cached, "equals", {value}).strictEquals(false)) {
            later(gl(state), name.c_str(), [&] { const auto x = value.get("x"), y = value.get("y"), z = value.get("z"), w = value.get("w"); return Args{x, y, z, w}; });
            call(cached, "copy", {value});
        }
    } else if (name == "updateUBOMapping") {
        const auto group = arg(args, 0), program = arg(args, 1);
        auto mapping = call(state.get("uboProgramMap"), "get", {program});
        if (mapping.isUndefined()) { mapping = createNativeWeakMap(); call(state.get("uboProgramMap"), "set", {program, mapping}); }
        auto index = call(mapping, "get", {group});
        if (index.isUndefined()) {
            index = later(gl(state), "getUniformBlockIndex", [&] { return Args{program, group.get("name")}; });
            call(gl(state), "uniformBlockBinding", {program, index, index}); call(mapping, "set", {group, index});
        }
    } else if (name == "uniformBlockBinding") {
        const auto group = arg(args, 0), program = arg(args, 1), buffer = arg(args, 2);
        const auto mapping = call(state.get("uboProgramMap"), "get", {program}), index = call(mapping, "get", {group});
        if (!state.get("uboBindings").get(index).strictEquals(buffer)) {
            later(gl(state), "bindBufferBase", [&] { return Args{gl(state).get("UNIFORM_BUFFER"), index, buffer}; }); state.get("uboBindings").set(index, buffer);
        }
    } else if (name == "reset") reset(state);
    else {
        // JS-valued native throws preserve the original caught value/identity.
        // Runtime C++ errors use its existing native exception boundary.
        try { call(gl(state), name.c_str(), args); }
        catch (const JSValue& error) { ThreeUtils::error({"WebGLState:", error}); }
        catch (const JSTypeError& error) { ThreeUtils::error({"WebGLState:", exceptionValue(error, "TypeError")}); }
        catch (const JSRangeError& error) { ThreeUtils::error({"WebGLState:", exceptionValue(error, "RangeError")}); }
        catch (const std::exception& error) { ThreeUtils::error({"WebGLState:", exceptionValue(error, "Error")}); }
    }
    return {};
}
double parseFloat(const JSValue& value) {
    const auto units = jsUTF16(value.toString());
    std::size_t first = 0;
    while (first < units.size() && jsWhitespace(units[first])) ++first;
    const auto text = jsUTF8(std::u16string_view(units).substr(first));
    std::size_t index = 0;
    if (text.empty()) return std::numeric_limits<double>::quiet_NaN();
    const auto start = index;
    if (text[index] == '+' || text[index] == '-') ++index;
    if (text.compare(index, 8, "Infinity") == 0) return text[start] == '-' ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    bool digits = false;
    while (index < text.size() && text[index] >= '0' && text[index] <= '9') { ++index; digits = true; }
    if (index < text.size() && text[index] == '.') { ++index; while (index < text.size() && text[index] >= '0' && text[index] <= '9') { ++index; digits = true; } }
    if (!digits) return std::numeric_limits<double>::quiet_NaN();
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        const auto exponent = index++; if (index < text.size() && (text[index] == '+' || text[index] == '-')) ++index;
        const auto firstDigit = index; while (index < text.size() && text[index] >= '0' && text[index] <= '9') ++index;
        if (firstDigit == index) index = exponent;
    }
    return JSValue(text.substr(start, index - start)).toNumber();
}
JSValue constructor(const JSValue&, const Args& args, const Args& captures) {
    auto state = JSValue::object(nullptr); literal(state, "gl", arg(args, 0)); literal(state, "extensions", arg(args, 1));
    literal(state, "reversedDepthFunctions", captures[0]);
    const auto colors = makeBuffer(state, "color"), depths = makeBuffer(state, "depth"), stencils = makeBuffer(state, "stencil");
    literal(state, "colorBuffer", colors); literal(state, "depthBuffer", depths); literal(state, "stencilBuffer", stencils);
    for (const auto* name : {"uboBindings", "enabledCapabilities", "parameters", "currentBoundFramebuffers", "currentBoundTextures"}) literal(state, name, JSValue::object());
    literal(state, "uboProgramMap", createNativeWeakMap()); literal(state, "currentDrawbuffers", createNativeWeakMap()); literal(state, "defaultDrawbuffers", JSValue::array());
    literal(state, "currentProgram", nullptr); literal(state, "currentBlendingEnabled", false);
    for (const auto* name : {"currentBlending", "currentBlendEquation", "currentBlendSrc", "currentBlendDst", "currentBlendEquationAlpha",
                             "currentBlendSrcAlpha", "currentBlendDstAlpha", "currentFlipSided", "currentCullFace", "currentLineWidth",
                             "currentPolygonOffsetFactor", "currentPolygonOffsetUnits", "currentTextureSlot"}) literal(state, name, nullptr);
    literal(state, "currentBlendColor", color()); literal(state, "currentBlendAlpha", 0); literal(state, "currentPremultipledAlpha", false);
    literal(state, "maxTextures", later(gl(state), "getParameter", [&] { return Args{gl(state).get("MAX_COMBINED_TEXTURE_IMAGE_UNITS")}; }));
    bool widthAvailable = false; double version = 0;
    const auto glVersion = later(gl(state), "getParameter", [&] { return Args{gl(state).get("VERSION")}; });
    if (!call(glVersion, "indexOf", {"WebGL"}).strictEquals(-1)) {
        const auto match = call(createNativeRegExp("^WebGL (\\d)"), "exec", {glVersion}); version = parseFloat(match.get("1")); widthAvailable = version >= 1;
    } else if (!call(glVersion, "indexOf", {"OpenGL ES"}).strictEquals(-1)) {
        const auto match = call(createNativeRegExp("^OpenGL ES (\\d)"), "exec", {glVersion}); version = parseFloat(match.get("1")); widthAvailable = version >= 2;
    }
    literal(state, "lineWidthAvailable", widthAvailable);
    const auto scissor = later(gl(state), "getParameter", [&] { return Args{gl(state).get("SCISSOR_BOX")}; });
    const auto viewport = later(gl(state), "getParameter", [&] { return Args{gl(state).get("VIEWPORT")}; });
    const auto currentScissor = call(vector(), "fromArray", {scissor}), currentViewport = call(vector(), "fromArray", {viewport});
    literal(state, "currentScissor", currentScissor); literal(state, "currentViewport", currentViewport);
    auto textures = JSValue::object(); literal(state, "emptyTextures", textures);
    for (const auto& [typeName, targetName, count, dimensions] : std::initializer_list<std::tuple<const char*, const char*, double, JSValue>>{
             {"TEXTURE_2D", "TEXTURE_2D", 1, {}}, {"TEXTURE_CUBE_MAP", "TEXTURE_CUBE_MAP_POSITIVE_X", 6, {}},
             {"TEXTURE_2D_ARRAY", "TEXTURE_2D_ARRAY", 1, 1}, {"TEXTURE_3D", "TEXTURE_3D", 1, 1}}) {
        const auto key = gl(state).get(typeName).toPropertyKey(), type = gl(state).get(typeName), target = gl(state).get(targetName);
        const auto texture = createTexture(state, type, target, count, dimensions); textures.set(key, texture);
    }
    call(colors, "setClear", {0, 0, 0, 1}); call(depths, "setClear", {1}); call(stencils, "setClear", {0});
    enable(state, gl(state).get("DEPTH_TEST")); call(depths, "setFunc", {LessEqualDepth});
    flipSided(state, false); cullFace(state, CullFaceBack); enable(state, gl(state).get("CULL_FACE")); blending(state, {NoBlending});
    auto equations = JSValue::object(); literal(state, "equationToGL", equations);
    for (const auto& [value, name] : std::initializer_list<std::pair<int, const char*>>{
             {AddEquation, "FUNC_ADD"}, {SubtractEquation, "FUNC_SUBTRACT"}, {ReverseSubtractEquation, "FUNC_REVERSE_SUBTRACT"},
             {MinEquation, "MIN"}, {MaxEquation, "MAX"}}) {
        const auto key = JSValue(value).toPropertyKey(), rhs = gl(state).get(name); equations.set(key, rhs);
    }
    auto factors = JSValue::object(); literal(state, "factorToGL", factors);
    for (const auto& [value, name] : std::initializer_list<std::pair<int, const char*>>{
             {ZeroFactor, "ZERO"}, {OneFactor, "ONE"}, {SrcColorFactor, "SRC_COLOR"}, {SrcAlphaFactor, "SRC_ALPHA"},
             {SrcAlphaSaturateFactor, "SRC_ALPHA_SATURATE"}, {DstColorFactor, "DST_COLOR"}, {DstAlphaFactor, "DST_ALPHA"},
             {OneMinusSrcColorFactor, "ONE_MINUS_SRC_COLOR"}, {OneMinusSrcAlphaFactor, "ONE_MINUS_SRC_ALPHA"},
             {OneMinusDstColorFactor, "ONE_MINUS_DST_COLOR"}, {OneMinusDstAlphaFactor, "ONE_MINUS_DST_ALPHA"},
             {ConstantColorFactor, "CONSTANT_COLOR"}, {OneMinusConstantColorFactor, "ONE_MINUS_CONSTANT_COLOR"},
             {ConstantAlphaFactor, "CONSTANT_ALPHA"}, {OneMinusConstantAlphaFactor, "ONE_MINUS_CONSTANT_ALPHA"}}) {
        const auto key = JSValue(value).toPropertyKey(), rhs = gl(state).get(name); factors.set(key, rhs);
    }
    auto result = JSValue::object(), buffers = JSValue::object(); literal(buffers, "color", colors); literal(buffers, "depth", depths); literal(buffers, "stencil", stencils);
    literal(result, "buffers", buffers);
    for (const auto& m : std::initializer_list<Method>{
        {"enable", 1, WebGLStateSource::enable}, {"disable", 1, WebGLStateSource::disable},
        {"bindFramebuffer", 2, WebGLStateSource::bindFramebuffer}, {"drawBuffers", 2, WebGLStateSource::drawBuffers},
        {"useProgram", 1, WebGLStateSource::useProgram}, {"setBlending", 10, WebGLStateSource::setBlending},
        {"setMaterial", 2, WebGLStateSource::setMaterial}, {"setFlipSided", 1, WebGLStateSource::setFlipSided},
        {"setCullFace", 1, WebGLStateSource::setCullFace}, {"setLineWidth", 1, WebGLStateSource::setLineWidth},
        {"setPolygonOffset", 3, WebGLStateSource::setPolygonOffset}, {"setScissorTest", 1, WebGLStateSource::setScissorTest},
        {"activeTexture", 1, WebGLStateSource::activeTexture}, {"bindTexture", 3, WebGLStateSource::bindTexture}, {"unbindTexture", 0, WebGLStateSource::unbindTexture},
        {"compressedTexImage2D", 0, WebGLStateSource::compressedTexImage2D}, {"compressedTexImage3D", 0, WebGLStateSource::compressedTexImage3D},
        {"texImage2D", 0, WebGLStateSource::texImage2D}, {"texImage3D", 0, WebGLStateSource::texImage3D},
        {"pixelStorei", 2, WebGLStateSource::pixelStorei}, {"getParameter", 1, WebGLStateSource::getParameter},
        {"updateUBOMapping", 2, WebGLStateSource::updateUBOMapping}, {"uniformBlockBinding", 3, WebGLStateSource::uniformBlockBinding},
        {"texStorage2D", 0, WebGLStateSource::texStorage2D}, {"texStorage3D", 0, WebGLStateSource::texStorage3D},
        {"texSubImage2D", 0, WebGLStateSource::texSubImage2D}, {"texSubImage3D", 0, WebGLStateSource::texSubImage3D},
        {"compressedTexSubImage2D", 0, WebGLStateSource::compressedTexSubImage2D}, {"compressedTexSubImage3D", 0, WebGLStateSource::compressedTexSubImage3D},
        {"scissor", 1, WebGLStateSource::scissor}, {"viewport", 1, WebGLStateSource::viewport}, {"reset", 0, WebGLStateSource::reset}}) {
        auto function = JSValue::constructor(method, {state, m.name}, m.name, m.arity, JSValue::ConstructorKind::Callable);
        function.setFunctionSourceText(m.source); literal(result, m.name, function);
    }
    return result;
}
} // namespace
JSValue nativeWebGLStateConstructor() {
    static thread_local JSWeakValue weak;
    static thread_local bool initializing = false;
    if (const auto value = weak.lock(); value && value->isCallable()) return *value;
    JSModuleInitializationScope scope(initializing, "WebGLState");
    auto function = JSValue::constructor(constructor, {ThreeUtils::ReversedDepthFuncs.object()}, "WebGLState", 2, JSValue::ConstructorKind::Callable);
    function.setFunctionSourceText(WebGLStateSource::WebGLState); function.retainForHeapLifetime(); weak = JSWeakValue(function); return function;
}
WebGLState::WebGLState(const JSValue& commands, const JSValue& extensions) {
    static_cast<void>(JSWeakValue(commands)); static_cast<void>(JSWeakValue(extensions));
    object_ = nativeWebGLStateConstructor().call({}, {commands, extensions});
}
JSValue WebGLState::invoke(const char* name, const std::vector<JSValue>& arguments) const {
    static_cast<void>(JSWeakValue(object_)); for (const auto& value : arguments) static_cast<void>(JSWeakValue(value));
    return object_.get(name).call(object_, arguments);
}
#define ONE(name) JSValue WebGLState::name(const JSValue& value) { return invoke(#name, {value}); }
ONE(enable) ONE(disable) ONE(useProgram) ONE(setFlipSided) ONE(setCullFace) ONE(setLineWidth) ONE(setScissorTest) ONE(activeTexture) ONE(getParameter) ONE(scissor) ONE(viewport)
#undef ONE
#define TWO(name) JSValue WebGLState::name(const JSValue& a, const JSValue& b) { return invoke(#name, {a, b}); }
TWO(bindFramebuffer) TWO(drawBuffers) TWO(setMaterial) TWO(pixelStorei) TWO(updateUBOMapping)
#undef TWO
#define THREE(name) JSValue WebGLState::name(const JSValue& a, const JSValue& b, const JSValue& c) { return invoke(#name, {a, b, c}); }
THREE(setPolygonOffset) THREE(bindTexture) THREE(uniformBlockBinding)
#undef THREE
JSValue WebGLState::setBlending(const JSValue& a, const JSValue& b, const JSValue& c, const JSValue& d, const JSValue& e,
                                const JSValue& f, const JSValue& g, const JSValue& h, const JSValue& i, const JSValue& j) { return invoke("setBlending", {a, b, c, d, e, f, g, h, i, j}); }
JSValue WebGLState::unbindTexture() { return invoke("unbindTexture"); }
JSValue WebGLState::reset() { return invoke("reset"); }
#define TEXTURE(name) JSValue WebGLState::name(const std::vector<JSValue>& args) { return invoke(#name, args); }
TEXTURE(compressedTexImage2D) TEXTURE(compressedTexImage3D) TEXTURE(texImage2D) TEXTURE(texImage3D) TEXTURE(texStorage2D) TEXTURE(texStorage3D)
TEXTURE(texSubImage2D) TEXTURE(texSubImage3D) TEXTURE(compressedTexSubImage2D) TEXTURE(compressedTexSubImage3D)
#undef TEXTURE

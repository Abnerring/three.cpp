#include "nodes/accessors/TextureBicubic.h"

#include "nodes/accessors/TextureNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/utils/MaxMipLevelNode.h"

namespace {
using namespace TSLFunctionRuntime;

constexpr double kBC = 1.0 / 6.0;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue operation(const char* name) {
    return nativeTSLBaseBinding(name);
}

JSValue math(const char* name) {
    return nativeMathNodeBinding(name);
}

JSValue w0(const JSValue& a) {
    auto inner = invoke(a, "negate")({});
    inner = invoke(inner, "add")({3.0});
    inner = invoke(a, "mul")({inner});
    inner = invoke(inner, "sub")({3.0});
    inner = invoke(inner, "add")({1.0});
    return operation("mul").call({}, {kBC, inner});
}

JSValue w1(const JSValue& a) {
    auto inner = operation("mul").call({}, {3.0, a});
    inner = invoke(inner, "sub")({6.0});
    inner = invoke(a, "mul")({inner});
    inner = invoke(inner, "add")({4.0});
    inner = invoke(a, "mul")({inner});
    return operation("mul").call({}, {kBC, inner});
}

JSValue w2(const JSValue& a) {
    auto inner = operation("mul").call({}, {-3.0, a});
    inner = invoke(inner, "add")({3.0});
    inner = invoke(a, "mul")({inner});
    inner = invoke(inner, "add")({3.0});
    inner = invoke(a, "mul")({inner});
    inner = invoke(inner, "add")({1.0});
    return operation("mul").call({}, {kBC, inner});
}

JSValue w3(const JSValue& a) {
    return operation("mul").call({}, {kBC, math("pow").call({}, {a, 3})});
}

struct BicubicWeights {
    JSValue g0;
    JSValue g1;
    JSValue h0;
    JSValue h1;
};

BicubicWeights bicubicWeights(const JSValue& a) {
    const auto w0a = w0(a);
    const auto w1a = w1(a);
    const auto w2a = w2(a);
    const auto w3a = w3(a);
    const auto g0a = invoke(w0a, "add")({w1a});
    const auto g1a = invoke(w2a, "add")({w3a});
    const auto h0a = operation("add").call({}, {-1.0, invoke(w1a, "div")({g0a})});
    const auto h1a = operation("add").call({}, {1.0, invoke(w3a, "div")({g1a})});
    return {g0a, g1a, h0a, h1a};
}

JSValue bicubic(const JSValue& textureNode, const JSValue& p0, const JSValue& p3,
                const JSValue& g0, const JSValue& g1, const JSValue& lod) {
    const auto p1 = nativeTSLBaseBinding("vec2").call({}, {p3.get("x"), p0.get("y")});
    const auto p2 = nativeTSLBaseBinding("vec2").call({}, {p0.get("x"), p3.get("y")});

    const auto sample0 = invoke(invoke(textureNode, "sample")({p0}), "level")({lod});
    const auto sample1 = invoke(invoke(textureNode, "sample")({p1}), "level")({lod});
    const auto sample2 = invoke(invoke(textureNode, "sample")({p2}), "level")({lod});
    const auto sample3 = invoke(invoke(textureNode, "sample")({p3}), "level")({lod});
    const auto a = invoke(g0.get("y"), "mul")({operation("add").call({}, {
        invoke(g0.get("x"), "mul")({sample0}), invoke(g1.get("x"), "mul")({sample1})})});
    const auto b = invoke(g1.get("y"), "mul")({operation("add").call({}, {
        invoke(g0.get("x"), "mul")({sample2}), invoke(g1.get("x"), "mul")({sample3})})});
    return invoke(a, "add")({b});
}

JSValue textureBicubicLevelBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto textureNode = inputs.get(JSValue(0));
    const auto lodNode = inputs.get(JSValue(1));
    const auto vec2 = nativeTSLBaseBinding("vec2");
    const auto vec4 = nativeTSLBaseBinding("vec4");
    const auto integer = nativeTSLBaseBinding("int");

    const auto fLodSize = vec2.call({}, {invoke(textureNode, "size")({integer.call({}, {lodNode})})});
    const auto cLodSize = vec2.call({}, {invoke(textureNode, "size")({
        integer.call({}, {invoke(lodNode, "add")({1.0})})})});
    const auto lodSize = vec4.call({}, {fLodSize, cLodSize});
    const auto lodSizeInv = operation("div").call({}, {1.0, lodSize});
    const auto uvScaled = invoke(textureNode.get("uvNode").get("xyxy"), "mul")({lodSize});
    const auto scaled = invoke(uvScaled, "add")({0.5});
    const auto iuv = math("floor").call({}, {scaled});
    const auto fuv = math("fract").call({}, {scaled});
    const auto weights = bicubicWeights(fuv);
    const auto p0 = invoke(invoke(iuv, "add")({weights.h0}), "sub")({0.5});
    const auto p3 = invoke(invoke(iuv, "add")({weights.h1}), "sub")({0.5});
    const auto p0Scaled = invoke(p0, "mul")({lodSizeInv});
    const auto p3Scaled = invoke(p3, "mul")({lodSizeInv});
    const auto fSample = bicubic(textureNode, p0Scaled.get("xy"), p3Scaled.get("xy"),
                                 weights.g0.get("xy"), weights.g1.get("xy"), math("floor").call({}, {lodNode}));
    const auto cSample = bicubic(textureNode, p0Scaled.get("zw"), p3Scaled.get("zw"),
                                 weights.g0.get("zw"), weights.g1.get("zw"), math("ceil").call({}, {lodNode}));
    return invoke(math("fract").call({}, {lodNode}), "mix")({fSample, cSample});
}

JSValue textureBicubicBody(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto textureNode = inputs.get(JSValue(0));
    const auto strength = inputs.get(JSValue(1));
    const auto lod = invoke(strength, "mul")({nativeMaxMipLevelNodeBinding("maxMipLevel").call({}, {textureNode})});
    return captures[0].call({}, {textureNode, lod});
}
} // namespace

JSValue nativeTextureBicubicModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "TextureBicubic module");
    nativeTextureNodeModule();
    nativeMaxMipLevelNodeModule();

    auto levelCallback = JSValue::function(textureBicubicLevelBody, {}, "", 1);
    levelCallback.setFunctionSourceText("( [ textureNode, lodNode ] ) => {\r\n\r\n\tconst fLodSize = vec2( textureNode.size( int( lodNode ) ) );\r\n\tconst cLodSize = vec2( textureNode.size( int( lodNode.add( 1.0 ) ) ) );\r\n\tconst lodSize = vec4( fLodSize, cLodSize );\r\n\tconst lodSizeInv = div( 1.0, lodSize );\r\n\tconst uvScaled = textureNode.uvNode.xyxy.mul( lodSize ).add( 0.5 );\r\n\tconst iuv = floor( uvScaled );\r\n\tconst fuv = fract( uvScaled );\r\n\r\n\tconst { g0, g1, h0, h1 } = bicubicWeights( fuv );\r\n\r\n\tconst p0 = iuv.add( h0 ).sub( 0.5 ).mul( lodSizeInv );\r\n\tconst p3 = iuv.add( h1 ).sub( 0.5 ).mul( lodSizeInv );\r\n\r\n\tconst fSample = bicubic( textureNode, p0.xy, p3.xy, g0.xy, g1.xy, floor( lodNode ) );\r\n\tconst cSample = bicubic( textureNode, p0.zw, p3.zw, g0.zw, g1.zw, ceil( lodNode ) );\r\n\r\n\treturn fract( lodNode ).mix( fSample, cSample );\r\n\r\n}");
    const auto level = nativeTSLBaseBinding("Fn").call({}, {levelCallback});

    auto bicubicCallback = JSValue::function(textureBicubicBody, {level}, "", 1);
    bicubicCallback.setFunctionSourceText("( [ textureNode, strength ] ) => {\r\n\r\n\tconst lod = strength.mul( maxMipLevel( textureNode ) );\r\n\r\n\treturn textureBicubicLevel( textureNode, lod );\r\n\r\n}");
    const auto bicubicFn = nativeTSLBaseBinding("Fn").call({}, {bicubicCallback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "textureBicubicLevel", level);
    own(bindings, "textureBicubic", bicubicFn);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);

    auto module = JSValue::object(nullptr);
    own(module, "textureBicubicLevel", level);
    own(module, "textureBicubic", bicubicFn);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeTextureBicubicBinding(const std::string& name) {
    const auto keeper = nativeTextureBicubicModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeTextureBicubicLevel() {
    return nativeTextureBicubicBinding("textureBicubicLevel");
}

JSValue nativeTextureBicubic() {
    return nativeTextureBicubicBinding("textureBicubic");
}

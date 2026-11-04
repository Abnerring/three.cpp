#include "nodes/utils/TriplanarTextures.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    const auto inputs = arg(args, 0);
    const auto textureXNode = inputs.get(JSValue(0));
    auto textureYNode = inputs.get(JSValue(1));
    if (textureYNode.isUndefined()) textureYNode = JSValue(nullptr);
    auto textureZNode = inputs.get(JSValue(2));
    if (textureZNode.isUndefined()) textureZNode = JSValue(nullptr);
    auto scaleNode = inputs.get(JSValue(3));
    if (scaleNode.isUndefined()) scaleNode = table.get("float").call({}, {1});
    auto positionNode = inputs.get(JSValue(4));
    if (positionNode.isUndefined()) positionNode = table.get("positionLocal");
    auto normalNode = inputs.get(JSValue(5));
    if (normalNode.isUndefined()) normalNode = table.get("normalLocal");

    auto bf = invoke(invoke(normalNode, "abs")(), "normalize")();
    const auto one = table.get("vec3").call({}, {1});
    bf = invoke(bf, "div")({invoke(bf, "dot")({one})});

    const auto tx = invoke(positionNode.get("yz"), "mul")({scaleNode});
    const auto ty = invoke(positionNode.get("zx"), "mul")({scaleNode});
    const auto tz = invoke(positionNode.get("xy"), "mul")({scaleNode});

    const auto textureX = textureXNode.get("value");
    const auto textureY = textureYNode.isNull() ? textureX : textureYNode.get("value");
    const auto textureZ = textureZNode.isNull() ? textureX : textureZNode.get("value");

    const auto cx = invoke(table.get("texture").call({}, {textureX, tx}), "mul")({bf.get("x")});
    const auto cy = invoke(table.get("texture").call({}, {textureY, ty}), "mul")({bf.get("y")});
    const auto cz = invoke(table.get("texture").call({}, {textureZ, tz}), "mul")({bf.get("z")});

    return table.get("add").call({}, {cx, cy, cz});
}

constexpr const char* kSource =
    "( [ textureXNode, textureYNode = null, textureZNode = null, scaleNode = float( 1 ), positionNode = positionLocal, normalNode = normalLocal ] ) => {\r\n\r\n"
    "\t// Reference: https://github.com/keijiro/StandardTriplanar\r\n\r\n"
    "\t// Blending factor of triplanar mapping\r\n"
    "\tlet bf = normalNode.abs().normalize();\r\n"
    "\tbf = bf.div( bf.dot( vec3( 1.0 ) ) );\r\n\r\n"
    "\t// Triplanar mapping\r\n"
    "\tconst tx = positionNode.yz.mul( scaleNode );\r\n"
    "\tconst ty = positionNode.zx.mul( scaleNode );\r\n"
    "\tconst tz = positionNode.xy.mul( scaleNode );\r\n\r\n"
    "\t// Base color\r\n"
    "\tconst textureX = textureXNode.value;\r\n"
    "\tconst textureY = textureYNode !== null ? textureYNode.value : textureX;\r\n"
    "\tconst textureZ = textureZNode !== null ? textureZNode.value : textureX;\r\n\r\n"
    "\tconst cx = texture( textureX, tx ).mul( bf.x );\r\n"
    "\tconst cy = texture( textureY, ty ).mul( bf.y );\r\n"
    "\tconst cz = texture( textureZ, tz ).mul( bf.z );\r\n\r\n"
    "\treturn add( cx, cy, cz );\r\n\r\n"
    "}";

} // namespace

JSValue nativeTriplanarTexturesModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "TriplanarTextures module");
    nativeOperatorNodeModule();
    nativeNormalModule();
    nativePositionModule();
    nativeTextureNodeModule();
    nativeTSLBase();

    auto table = JSValue::object(nullptr);
    own(table, "add", nativeOperatorNodeBinding("add"));
    own(table, "normalLocal", nativeNormalBinding("normalLocal"));
    own(table, "positionLocal", nativePositionBinding("positionLocal"));
    own(table, "texture", nativeTextureNodeBinding("texture"));
    own(table, "float", nativeTSLBaseBinding("float"));
    own(table, "vec3", nativeTSLBaseBinding("vec3"));

    auto callback = JSValue::function(body, {table}, "", 1);
    callback.setFunctionSourceText(kSource);
    const auto textures = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "triplanarTextures", textures);
    own(bindings, "triplanarTexture", JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures[0].call({}, args);
        }, {textures}, "triplanarTexture", 0));
    auto alias = bindings.get("triplanarTexture");
    alias.setFunctionSourceText("( ...params ) => triplanarTextures( ...params )");

    auto module = JSValue::object(nullptr);
    own(module, "triplanarTextures", textures);
    own(module, "triplanarTexture", alias);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeTriplanarTexturesBinding(const std::string& name) {
    const auto keeper = nativeTriplanarTexturesModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeTriplanarTextures() {
    return nativeTriplanarTexturesBinding("triplanarTextures");
}

JSValue nativeTriplanarTexture() {
    return nativeTriplanarTexturesBinding("triplanarTexture");
}
